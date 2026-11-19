#include "ScratchEngine.h"

#include <algorithm>
#include <cmath>

namespace bs
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

//------------------------------------------------------------------------------
struct Rng
{
    uint32_t s;
    explicit Rng (uint32_t seed) : s (seed * 2654435761u + 0x9e3779b9u) { if (s == 0) s = 1; next(); next(); }
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float u()                     { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
    float range (float a, float b) { return a + (b - a) * u(); }
    float bi()                    { return u() * 2.0f - 1.0f; }
    bool  chance (float p)        { return u() < p; }
};

//------------------------------------------------------------------------------
struct Ctx
{
    const Audio*    src;
    size_t          rs;      // region start (samples)
    size_t          len;     // region length (samples)
    double          sr;
    Audio*          out;
    Rng*            rng;
    const Settings* st;
};

// 4-point Hermite read from a region with wrap-around. pos is region-relative.
inline float readCh (const std::vector<float>& d, size_t rs, size_t len, double pos)
{
    const double L = (double) len;
    double p = std::fmod (pos, L);
    if (p < 0.0) p += L;
    const long i = (long) p;
    const float f = (float) (p - (double) i);
    const long l = (long) len;

    auto at = [&] (long k) -> float
    {
        long j = (i + k) % l;
        if (j < 0) j += l;
        return d[rs + (size_t) j];
    };

    const float y0 = at (-1), y1 = at (0), y2 = at (1), y3 = at (2);
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * f + c2) * f + c1) * f + y1;
}

inline void readStereo (const Ctx& c, double pos, float& l, float& r)
{
    l = readCh (c.src->ch[0], c.rs, c.len, pos);
    r = readCh (c.src->ch[1], c.rs, c.len, pos);
}

inline void fadeEdges (Audio& a, size_t o0, size_t o1, size_t f)
{
    if (o1 <= o0) return;
    f = std::min (f, (o1 - o0) / 2);
    for (size_t i = 0; i < f; ++i)
    {
        const float g = (float) (i + 1) / (float) (f + 1);
        for (auto& ch : a.ch)
        {
            ch[o0 + i]     *= g;
            ch[o1 - 1 - i] *= g;
        }
    }
}

inline float softSat (float x, float drive)
{
    return std::tanh (x * drive) / std::tanh (drive);
}

inline size_t msToSamples (double ms, double sr) { return (size_t) std::max (1.0, ms * 0.001 * sr); }

//------------------------------------------------------------------------------
// Plain playback at a fixed rate
void renderPlay (Ctx& c, size_t o0, size_t o1, double& pos, double rate)
{
    for (size_t i = o0; i < o1; ++i)
    {
        float l, r;
        readStereo (c, pos, l, r);
        c.out->ch[0][i] = l;
        c.out->ch[1][i] = r;
        pos += rate;
    }
}

//------------------------------------------------------------------------------
// SCRATCH: hand-motion keyframes + crossfader gate
void renderScratch (Ctx& c, size_t o0, size_t o1, double& pos)
{
    const Settings& st = *c.st;
    Rng& rng = *c.rng;
    if (o1 <= o0) return;

    const size_t n = o1 - o0;
    const double stepLen = std::max (24.0, c.sr * 60.0 / st.bpm / st.stepsPerBeat);
    const int nSteps = std::max (1, (int) std::ceil ((double) n / stepLen));

    const double rateMul = std::pow (2.0, st.pitchSemis / 12.0) * (0.5 + 2.0 * st.speed);
    const double travelBase = stepLen * rateMul * (0.35 + 0.9 * st.intensity);

    struct Key { double t, p; };
    std::vector<Key> keys;
    keys.push_back ({ 0.0, pos });

    double pivot = pos;
    double cur   = pos;
    int    dir   = 1;

    std::vector<float> gain (n, 1.0f);
    const int stepsPerBar = (int) std::round (4.0 * st.stepsPerBeat);
    const float gateProb = st.cuts <= 0 ? 0.0f
                         : std::min (1.0f, (float) st.cuts / (float) std::max (1, stepsPerBar));
    const float silentFrac = 0.3f + 0.5f * st.intensity;

    for (int s = 0; s < nSteps; ++s)
    {
        const double base = s * stepLen;
        double jitter = 0.0;
        switch (st.variation)
        {
            case Variation::Human:  jitter = rng.bi() * 0.08 * stepLen; break;
            case Variation::Tight:  jitter = 0.0; break;
            case Variation::Swing:  jitter = (s % 2 == 1) ? 0.22 * stepLen : 0.0; break;
            case Variation::Random: jitter = rng.bi() * 0.30 * stepLen; break;
        }

        int k = 1;
        if (rng.chance (st.complexity))        ++k;
        if (rng.chance (st.complexity * 0.5f)) ++k;

        for (int j = 0; j < k; ++j)
        {
            double t = base + jitter + (double) j * stepLen / (double) k;
            t = std::max (t, keys.back().t + 8.0);
            if (t >= (double) n) break;

            if (rng.chance (0.15f + 0.6f * st.complexity) ? rng.chance (0.5f) : true)
                dir = -dir;

            double humanAmt = 1.0;
            if (st.variation == Variation::Human)  humanAmt = 0.85 + 0.3 * rng.u();
            if (st.variation == Variation::Random) humanAmt = 0.5 + 1.0 * rng.u();

            const double amp = travelBase / (double) k * (0.7 + 0.6 * rng.u()) * humanAmt;
            cur = pivot + (dir > 0 ? amp : -0.35 * amp);
            keys.push_back ({ t, cur });
        }

        pivot += travelBase * 0.10 * (0.5 + st.speed);

        if (rng.chance (gateProb))
        {
            const double a = base + stepLen * (1.0 - silentFrac);
            const double b = base + stepLen;
            const size_t ia = (size_t) std::clamp (a, 0.0, (double) n);
            const size_t ib = (size_t) std::clamp (b, 0.0, (double) n);
            for (size_t i = ia; i < ib; ++i) gain[i] = 0.0f;
        }
    }
    keys.push_back ({ (double) n, cur });

    // smooth the gate (2 ms) to avoid clicks
    {
        const float coef = 1.0f - std::exp (-1.0f / (0.002f * (float) c.sr));
        float g = 1.0f;
        for (size_t i = 0; i < n; ++i) { g += (gain[i] - g) * coef; gain[i] = g; }
    }

    const float drive = 1.0f + st.intensity * 1.5f;
    size_t ki = 0;
    for (size_t i = 0; i < n; ++i)
    {
        while (ki + 2 < keys.size() && (double) i >= keys[ki + 1].t) ++ki;
        const Key& a = keys[ki];
        const Key& b = keys[ki + 1];
        const double span = std::max (1.0, b.t - a.t);
        const double u = std::clamp (((double) i - a.t) / span, 0.0, 1.0);
        const double sm = u * u * (3.0 - 2.0 * u);
        const double e = u + (sm - u) * 0.6;
        const double p = a.p + (b.p - a.p) * e;

        float l, r;
        readStereo (c, p, l, r);
        c.out->ch[0][o0 + i] = softSat (l, drive) * gain[i];
        c.out->ch[1][o0 + i] = softSat (r, drive) * gain[i];
    }
    pos = cur;
    fadeEdges (*c.out, o0, o1, msToSamples (1.5, c.sr));
}

//------------------------------------------------------------------------------
// STUTTER: repeat a slice that gets progressively shorter
void renderStutter (Ctx& c, size_t o0, size_t o1, double& pos)
{
    const Settings& st = *c.st;
    if (o1 <= o0) return;

    const double stepLen = std::max (48.0, c.sr * 60.0 / st.bpm / st.stepsPerBeat);
    const double srcStart = pos;
    double slice = stepLen;
    const int repsPerLevel = std::max (2, (int) (4.0f - 2.0f * st.intensity));

    size_t i = o0;
    int rep = 0;
    while (i < o1)
    {
        const size_t len = std::min ((size_t) std::max (32.0, slice), o1 - i);
        const double rate = std::pow (2.0, (st.pitchSemis + st.intensity * (float) rep * 0.35f) / 12.0);
        const size_t open = (size_t) ((double) len * 0.8);

        for (size_t j = 0; j < len; ++j)
        {
            float l, r;
            readStereo (c, srcStart + (double) j * rate, l, r);
            float g = 1.0f;
            if (j >= open) g = 0.0f;
            c.out->ch[0][i + j] = l * g;
            c.out->ch[1][i + j] = r * g;
        }
        fadeEdges (*c.out, i, i + len, 48);
        // soften the gate edge inside the slice
        for (size_t j = (open > 48 ? open - 48 : 0); j < open && j < len; ++j)
        {
            const float g = (float) (open - j) / 48.0f;
            c.out->ch[0][i + j] *= std::min (1.0f, g);
            c.out->ch[1][i + j] *= std::min (1.0f, g);
        }

        i += len;
        ++rep;
        if (rep % repsPerLevel == 0 && slice > stepLen / 8.0) slice *= 0.5;
    }
    pos = srcStart + stepLen;
}

//------------------------------------------------------------------------------
// REVERSE
void renderReverse (Ctx& c, size_t o0, size_t o1, double& pos)
{
    if (o1 <= o0) return;
    const double rate = std::pow (2.0, c.st->pitchSemis / 12.0);
    for (size_t i = o0; i < o1; ++i)
    {
        float l, r;
        readStereo (c, pos, l, r);
        c.out->ch[0][i] = l;
        c.out->ch[1][i] = r;
        pos -= rate;
    }
    fadeEdges (*c.out, o0, o1, msToSamples (2.0, c.sr));
}

//------------------------------------------------------------------------------
// GLITCH
void renderGlitch (Ctx& c, size_t o0, size_t o1, double& pos)
{
    const Settings& st = *c.st;
    Rng& rng = *c.rng;
    if (o1 <= o0) return;

    const double stepLen = std::max (64.0, c.sr * 60.0 / st.bpm / st.stepsPerBeat);
    size_t i = o0;
    while (i < o1)
    {
        const size_t len = std::min ((size_t) stepLen, o1 - i);
        const float roll = rng.u();
        const float act = roll * (0.6f + 0.8f * st.intensity);
        const double startPos = pos;

        if (act < 0.25f)            // repeat a short slice
        {
            const size_t sl = std::max<size_t> (24, len / 2);
            for (size_t j = 0; j < len; ++j)
            {
                float l, r;
                readStereo (c, startPos + (double) (j % sl), l, r);
                c.out->ch[0][i + j] = l; c.out->ch[1][i + j] = r;
            }
        }
        else if (act < 0.45f)       // reverse slice
        {
            for (size_t j = 0; j < len; ++j)
            {
                float l, r;
                readStereo (c, startPos + (double) len - (double) j, l, r);
                c.out->ch[0][i + j] = l; c.out->ch[1][i + j] = r;
            }
        }
        else if (act < 0.65f)       // pitch jump
        {
            const int semi = rng.chance (0.5f) ? (int) rng.range (3, 12) : -(int) rng.range (3, 12);
            const double rate = std::pow (2.0, semi / 12.0);
            for (size_t j = 0; j < len; ++j)
            {
                float l, r;
                readStereo (c, startPos + (double) j * rate, l, r);
                c.out->ch[0][i + j] = l; c.out->ch[1][i + j] = r;
            }
        }
        else if (act < 0.85f)       // bit-crush + sample-hold
        {
            const size_t hold = (size_t) rng.range (3, 14);
            float hl = 0, hr = 0;
            for (size_t j = 0; j < len; ++j)
            {
                if (j % hold == 0) readStereo (c, startPos + (double) j, hl, hr);
                const float q = 24.0f;
                c.out->ch[0][i + j] = std::round (hl * q) / q;
                c.out->ch[1][i + j] = std::round (hr * q) / q;
            }
        }
        else if (act < 1.0f)        // drop-out
        {
            for (size_t j = 0; j < len; ++j) { c.out->ch[0][i + j] = 0.0f; c.out->ch[1][i + j] = 0.0f; }
        }
        else                        // normal
        {
            for (size_t j = 0; j < len; ++j)
            {
                float l, r;
                readStereo (c, startPos + (double) j, l, r);
                c.out->ch[0][i + j] = l; c.out->ch[1][i + j] = r;
            }
        }
        fadeEdges (*c.out, i, i + len, 32);
        pos = startPos + (double) len;
        i += len;
    }
}

//------------------------------------------------------------------------------
// STOP TAPE
size_t stopDurationSamples (StopType type, double stopTimeSec, double sr)
{
    double d = stopTimeSec;
    switch (type)
    {
        case StopType::Vinyl:      d *= 0.85; break;
        case StopType::Tape:       break;
        case StopType::SlowMotion: d *= 1.7;  break;
        case StopType::Deep:       d *= 1.3;  break;
        case StopType::Hard:       d = 0.07;  break;
        case StopType::Reverse:    d *= 1.0;  break;
    }
    return (size_t) std::max (64.0, d * sr);
}

// The stop begins at o0 (rate 1) and lasts T samples, the rest of [o0,o1) is silence/noise floor.
void renderStop (Ctx& c, size_t o0, size_t o1, double& pos, StopType type, size_t T,
                 float pitchDrop, float curve, Texture tex)
{
    if (o1 <= o0) return;
    const size_t n = o1 - o0;
    T = std::min (T, n);
    Rng& rng = *c.rng;

    float lpAmount = 0.0f, drive = 1.0f;
    switch (type)
    {
        case StopType::Vinyl:      lpAmount = 0.15f; break;
        case StopType::Tape:       lpAmount = 0.50f; break;
        case StopType::SlowMotion: lpAmount = 0.80f; break;
        case StopType::Deep:       lpAmount = 1.0f; drive = 1.8f; break;
        case StopType::Hard:       break;
        case StopType::Reverse:    lpAmount = 0.25f; break;
    }

    float lp[2] = { 0.0f, 0.0f };
    float holdL = 0, holdR = 0; size_t holdCount = 0;
    double p = pos;
    const double invSr = 1.0 / c.sr;

    for (size_t i = 0; i < n; ++i)
    {
        float outL = 0.0f, outR = 0.0f;

        if (i < T)
        {
            const double x = (double) i / (double) T;
            double rate = stopRateAt (type, x, pitchDrop, curve);

            if (tex == Texture::Tape)
            {
                const double t = (double) i * invSr;
                rate *= 1.0 + 0.006 * std::sin (2.0 * kPi * 5.2 * t) + 0.003 * std::sin (2.0 * kPi * 0.9 * t);
            }

            float l, r;
            readStereo (c, p, l, r);
            p += rate;

            // gain: fade the tail
            float g = 1.0f;
            if (type == StopType::Hard)
                g = (x < 0.85) ? 1.0f : (float) std::max (0.0, 1.0 - (x - 0.85) / 0.15);
            else if (x > 0.75)
            {
                const double f = (x - 0.75) / 0.25;
                g = (float) std::max (0.0, 1.0 - f * f);
            }
            l *= g; r *= g;

            // low-pass that follows the speed
            const double arate = std::clamp (std::fabs (rate), 0.02, 1.0);
            const double fcOpen = 18000.0;
            const double fcClosed = 500.0 + 12000.0 * std::pow (arate, 1.5);
            const double fc = fcOpen + (fcClosed - fcOpen) * lpAmount;
            const float coef = (float) (1.0 - std::exp (-2.0 * kPi * std::min (fc, 0.45 * c.sr) * invSr));
            lp[0] += (l - lp[0]) * coef;
            lp[1] += (r - lp[1]) * coef;
            outL = lp[0]; outR = lp[1];

            if (tex == Texture::Digital && x > 0.15)
            {
                const size_t hold = 1 + (size_t) (x * 9.0);
                if (holdCount % hold == 0) { holdL = outL; holdR = outR; }
                ++holdCount;
                const float q = 64.0f;
                outL = std::round (holdL * q) / q;
                outR = std::round (holdR * q) / q;
            }

            if (drive > 1.0f) { outL = softSat (outL, drive); outR = softSat (outR, drive); }
        }

        if (tex == Texture::Vinyl)
        {
            float noise = rng.bi() * 0.0025f;
            if (rng.chance (0.0006f)) noise += rng.bi() * 0.18f;
            outL += noise; outR += noise * (0.8f + 0.2f * rng.u());
        }

        c.out->ch[0][o0 + i] = outL;
        c.out->ch[1][o0 + i] = outR;
    }
    pos = p;
}

//------------------------------------------------------------------------------
// TAPE RESTART: speed ramps from 0 to 1 and ends exactly on `pos`.
void renderRestart (Ctx& c, size_t o0, size_t o1, double& pos, Texture tex)
{
    if (o1 <= o0) return;
    const size_t n = o1 - o0;
    std::vector<double> rates (n);
    double total = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        const double x = (double) (i + 1) / (double) n;
        double r = std::pow (x, 1.6);
        if (tex == Texture::Tape)
            r *= 1.0 + 0.006 * std::sin (2.0 * kPi * 5.2 * (double) i / c.sr);
        rates[i] = r;
        total += r;
    }

    double p = pos - total;
    float lp[2] = { 0.0f, 0.0f };
    for (size_t i = 0; i < n; ++i)
    {
        const double x = (double) (i + 1) / (double) n;
        float l, r;
        readStereo (c, p, l, r);
        p += rates[i];

        const float g = (float) std::pow (x, 0.7);
        const double fc = 500.0 + 17500.0 * x * x;
        const float coef = (float) (1.0 - std::exp (-2.0 * kPi * fc / c.sr));
        lp[0] += (l * g - lp[0]) * coef;
        lp[1] += (r * g - lp[1]) * coef;
        c.out->ch[0][o0 + i] = lp[0];
        c.out->ch[1][o0 + i] = lp[1];
    }
    // pos stays where it was: playback after the restart continues from there
}

//------------------------------------------------------------------------------
void renderComboSegment (int idx, Ctx& c, size_t a, size_t b, double& pos)
{
    const size_t len = b - a;
    auto at = [&] (double f) { return a + (size_t) ((double) len * f); };
    const Settings& st = *c.st;

    switch (idx)
    {
        case 0: // Scratch + Stop Tape
        {
            const size_t s = at (0.70);
            renderScratch (c, a, s, pos);
            const size_t T = std::min (b - s, stopDurationSamples (st.stopType, st.stopTime, c.sr));
            renderStop (c, s, b, pos, st.stopType, T, st.pitchDrop, st.curve, st.texture);
            break;
        }
        case 1: // Vocal Stutter + Scratch
        {
            const size_t s = at (0.40);
            renderStutter (c, a, s, pos);
            renderScratch (c, s, b, pos);
            break;
        }
        case 2: // Reverse + Deep Stop
        {
            const size_t s = at (0.55);
            renderReverse (c, a, s, pos);
            // after reversing, continue forward for the deep stop
            renderStop (c, s, b, pos, StopType::Deep,
                        std::min (b - s, stopDurationSamples (StopType::Deep, st.stopTime, c.sr)),
                        st.pitchDrop, st.curve, st.texture);
            break;
        }
        default: // Scratch + Glitch + Restart
        {
            const size_t s1 = at (0.45), s2 = at (0.75);
            renderScratch (c, a, s1, pos);
            renderGlitch  (c, s1, s2, pos);
            renderRestart (c, s2, b, pos, st.texture);
            break;
        }
    }
    fadeEdges (*c.out, a, b, msToSamples (1.5, c.sr));
}
} // namespace


double stopRateAt (StopType type, double x, float pitchDrop, float curve)
{
    x = std::clamp (x, 0.0, 1.0);
    if (type == StopType::Reverse)
        return 1.0 - 2.6 * std::pow (x, 0.9 + (double) curve);

    double shapeMul = 1.0, extraDrop = 0.0, xe = 0.8;
    switch (type)
    {
        case StopType::Vinyl:      shapeMul = 1.0; break;
        case StopType::Tape:       shapeMul = 1.5; break;
        case StopType::SlowMotion: shapeMul = 0.8; break;
        case StopType::Deep:       shapeMul = 2.2; extraDrop = -12.0; break;
        case StopType::Hard:       shapeMul = 1.0; xe = 0.5; break;
        case StopType::Reverse:    break;
    }
    const double shape = (0.5 + 2.5 * (double) curve) * shapeMul;
    const double drop  = std::min (0.0f, pitchDrop) + extraDrop;
    if (x <= xe) return std::pow (2.0, drop * std::pow (x, shape) / 12.0);
    const double rxe = std::pow (2.0, drop * std::pow (xe, shape) / 12.0);
    return rxe * (1.0 - (x - xe) / (1.0 - xe));
}

//==============================================================================
size_t patternLengthSamples (const Settings& s, double sr)
{
    const double bpm = std::clamp (s.bpm, 30.0, 300.0);
    return (size_t) std::llround (s.durationBeats * 60.0 / bpm * sr);
}

Audio render (const Audio& src, size_t rs, size_t re, const Settings& settingsIn, double sr)
{
    Settings S = settingsIn;
    S.bpm = std::clamp (S.bpm, 30.0, 300.0);
    S.stopTime = std::clamp (S.stopTime, 0.05f, 8.0f);

    Audio out;
    const size_t N = patternLengthSamples (S, sr);
    out.resize (N);
    if (src.empty() || N == 0 || src.ch[1].size() != src.ch[0].size()) return out;

    re = std::min (re, src.size());
    rs = std::min (rs, re);
    if (re - rs < 256) { rs = 0; re = src.size(); }
    if (re - rs < 4)   return out;

    Rng rng (S.seed);
    Ctx c { &src, rs, re - rs, sr, &out, &rng, &S };
    double pos = 0.0;

    switch (S.mode)
    {
        case Mode::Scratch:
            renderScratch (c, 0, N, pos);
            break;

        case Mode::StopTape:
        {
            size_t T = stopDurationSamples (S.stopType, S.stopTime, sr);
            size_t Tr = S.restart ? (size_t) std::max (64.0, 0.6 * (double) T) : 0;
            size_t gap = S.restart ? (size_t) (0.1 * (double) T) : 0;

            // fit everything into the pattern (keep at least 10% playing before the stop)
            const double maxBlock = 0.9 * (double) N;
            const double want = (double) (T + gap + Tr);
            if (want > maxBlock)
            {
                const double k = maxBlock / want;
                T   = std::max<size_t> (64, (size_t) ((double) T * k));
                Tr  = S.restart ? std::max<size_t> (64, (size_t) ((double) Tr * k)) : 0;
                gap = (size_t) ((double) gap * k);
            }

            const size_t stopStart = N - Tr - gap - T;
            renderPlay (c, 0, stopStart, pos, 1.0);
            renderStop (c, stopStart, N - Tr, pos, S.stopType, T, S.pitchDrop, S.curve, S.texture);
            if (S.restart)
            {
                double target = 0.0; // loop start: seamless re-entry
                renderRestart (c, N - Tr, N, target, S.texture);
            }
            break;
        }

        case Mode::Combo:
        {
            std::vector<int> active;
            for (int i = 0; i < 4; ++i) if (S.combos[(size_t) i]) active.push_back (i);
            if (active.empty()) active.push_back (0);

            const size_t barLen = (size_t) (4.0 * 60.0 / S.bpm * sr);
            const size_t segLen = (S.durationBeats > 4.0 && barLen > 0) ? barLen : N;
            const size_t nSeg = std::max<size_t> (1, (N + segLen - 1) / segLen);
            const size_t offset = (size_t) (S.seed % active.size());

            for (size_t s = 0; s < nSeg; ++s)
            {
                const size_t a = s * segLen;
                const size_t b = (s == nSeg - 1) ? N : std::min (N, a + segLen);
                const int idx = active[(s + offset) % active.size()];
                renderComboSegment (idx, c, a, b, pos);
            }
            break;
        }
    }

    // loop-safe edges + gentle output safety
    fadeEdges (out, 0, N, msToSamples (1.5, sr));
    for (auto& ch : out.ch)
        for (auto& v : ch)
        {
            const float a = std::fabs (v);
            if (a > 0.9f) v = std::copysign (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f), v);
            if (! std::isfinite (v)) v = 0.0f;
        }
    return out;
}
} // namespace bs

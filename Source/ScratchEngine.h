#pragma once
//==============================================================================
//  BLESSED SCRATCH - DSP engine
//  Pure C++17 (no JUCE dependency) so it can be unit-tested on any platform.
//
//  The engine takes a captured vocal region and renders a complete pattern
//  (scratch / stop-tape / combo) of a given length in beats at a given BPM.
//==============================================================================
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace bs
{
struct Audio
{
    std::array<std::vector<float>, 2> ch;

    size_t size() const noexcept { return ch[0].size(); }
    bool   empty() const noexcept { return ch[0].empty(); }
    void   resize (size_t n)      { ch[0].assign (n, 0.0f); ch[1].assign (n, 0.0f); }
};

enum class Mode      { Scratch = 0, StopTape = 1, Combo = 2 };
enum class StopType  { Vinyl = 0, Tape, SlowMotion, Deep, Hard, Reverse };
enum class Texture   { Clean = 0, Vinyl, Tape, Digital };
enum class Variation { Human = 0, Tight, Swing, Random };

struct Settings
{
    Mode      mode          = Mode::Scratch;
    double    bpm           = 90.0;
    double    durationBeats = 16.0;   // pattern length
    double    stepsPerBeat  = 4.0;    // rhythm grid (1/16 = 4)

    // SCRATCH
    float     speed         = 0.5f;   // 0..1
    float     complexity    = 0.7f;   // 0..1
    float     intensity     = 0.75f;  // 0..1
    int       cuts          = 8;      // crossfader cuts per bar (0..32)
    Variation variation     = Variation::Human;
    int       pitchSemis    = 0;      // -12..12

    // STOP TAPE
    StopType  stopType      = StopType::Vinyl;
    float     stopTime      = 1.5f;   // seconds
    float     pitchDrop     = -12.0f; // semitones (<= 0)
    float     curve         = 0.5f;   // 0..1 braking curve
    Texture   texture       = Texture::Vinyl;
    bool      restart       = false;  // append a tape restart

    // COMBO FX
    std::array<bool, 4> combos { { true, true, true, true } };
    //  0 Scratch + Stop Tape   1 Vocal Stutter + Scratch
    //  2 Reverse + Deep Stop   3 Scratch + Glitch + Restart

    uint32_t  seed          = 1;
};

// Playback-speed multiplier of a stop at normalised time x (0..1). Also used by the UI curve view.
double stopRateAt (StopType type, double x, float pitchDrop, float curve);

// Number of output samples a pattern will have for the given settings.
size_t patternLengthSamples (const Settings& s, double sampleRate);

// Render a pattern from src[regionStart, regionEnd). Output is stereo and
// exactly patternLengthSamples() long. Safe to call from any non-realtime thread.
Audio render (const Audio& src, size_t regionStart, size_t regionEnd,
              const Settings& settings, double sampleRate);
} // namespace bs

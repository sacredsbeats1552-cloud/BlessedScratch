#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
const double kDurationBeats[] = { 2.0, 4.0, 8.0, 16.0, 32.0 };
const double kStepsPerBeat[]  = { 1.0, 2.0, 3.0, 4.0, 6.0, 8.0 };
const int    kCuts[]          = { 0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32 };

struct FactoryPreset
{
    const char* name;
    float speed, complexity, intensity;
    int duration, rhythm, cuts, variation, pitch, stopType;
    float stopTime, pitchDrop, curve;
    int texture;
};

// duration idx: 0=1/2 bar 1=1 bar 2=2 bars 3=4 bars 4=8 bars
// rhythm idx:   0=1/4 1=1/8 2=1/8T 3=1/16 4=1/16T 5=1/32
// cuts idx:     0,1,2,3,4,6,8,12,16,24,32      variation: 0=Human 1=Tight 2=Swing 3=Random
const FactoryPreset kFactory[] =
{
    { "Hip-Hop Classics", 0.50f, 0.60f, 0.75f, 3, 3, 6, 0, 0, 0, 1.5f, -12.0f, 0.50f, 1 },
    { "Boom Bap",         0.40f, 0.45f, 0.70f, 3, 1, 5, 2, 0, 1, 1.4f, -10.0f, 0.45f, 2 },
    { "Trap / Modern",    0.70f, 0.80f, 0.85f, 2, 4, 8, 3, 2, 3, 1.8f, -24.0f, 0.60f, 3 },
    { "Commercial",       0.45f, 0.35f, 0.60f, 3, 3, 4, 1, 0, 1, 1.2f,  -8.0f, 0.40f, 0 },
    { "DJ Cuts",          0.85f, 0.95f, 0.90f, 3, 5, 9, 0, 0, 4, 0.5f, -12.0f, 0.50f, 0 },
};
} // namespace

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout BlessedScratchProcessor::createLayout()
{
    using F  = AudioParameterFloat;
    using FA = AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pid = [] (const char* s) { return ParameterID { s, 1 }; };

    p.push_back (std::make_unique<AudioParameterChoice> (pid ("mode"), "Mode",
                 StringArray { "Scratch", "Stop Tape", "Combo FX" }, 0));

    p.push_back (std::make_unique<F> (pid ("speed"), "Speed", NormalisableRange<float> (0.0f, 1.0f), 0.5f,
        FA().withStringFromValueFunction ([] (float v, int) {
            return v < 0.25f ? String ("Slow") : v < 0.6f ? String ("Medium") : v < 0.85f ? String ("Fast") : String ("Insane"); })));
    p.push_back (std::make_unique<F> (pid ("complexity"), "Complexity", NormalisableRange<float> (0.0f, 1.0f), 0.8f,
        FA().withStringFromValueFunction ([] (float v, int) {
            return v < 0.34f ? String ("Low") : v < 0.67f ? String ("Medium") : String ("High"); })));
    p.push_back (std::make_unique<F> (pid ("intensity"), "Intensity", NormalisableRange<float> (0.0f, 1.0f), 0.75f,
        FA().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; })));
    p.push_back (std::make_unique<AudioParameterChoice> (pid ("duration"), "Duration",
                 StringArray { "1/2 Bar", "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 3));
    p.push_back (std::make_unique<AudioParameterChoice> (pid ("rhythm"), "Rhythm",
                 StringArray { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3));
    {
        StringArray c; for (int v : kCuts) c.add (String (v));
        p.push_back (std::make_unique<AudioParameterChoice> (pid ("cuts"), "Cuts", c, 6));   // 8
    }
    p.push_back (std::make_unique<AudioParameterChoice> (pid ("variation"), "Variation",
                 StringArray { "Human", "Tight", "Swing", "Random" }, 3));
    {
        StringArray c; for (int v = -12; v <= 12; ++v) c.add (String (v) + " st");
        p.push_back (std::make_unique<AudioParameterChoice> (pid ("pitch"), "Pitch", c, 12));   // 0 st
    }

    p.push_back (std::make_unique<AudioParameterChoice> (pid ("stopType"), "Stop Type",
                 StringArray { "Vinyl Stop", "Tape Stop", "Slow Motion", "Deep Stop", "Hard Stop", "Reverse Stop" }, 0));
    p.push_back (std::make_unique<F> (pid ("stopTime"), "Stop Time", NormalisableRange<float> (0.1f, 4.0f, 0.01f, 0.6f), 1.5f,
        FA().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + "s"; })));
    p.push_back (std::make_unique<F> (pid ("pitchDrop"), "Pitch Drop", NormalisableRange<float> (-36.0f, 0.0f, 1.0f), -12.0f,
        FA().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " st"; })));
    p.push_back (std::make_unique<AudioParameterChoice> (pid ("texture"), "Texture",
                 StringArray { "Clean", "Vinyl", "Tape", "Digital" }, 1));
    p.push_back (std::make_unique<F> (pid ("curve"), "Curve", NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    p.push_back (std::make_unique<AudioParameterBool> (pid ("restart"), "Tape Restart", false));

    p.push_back (std::make_unique<AudioParameterBool> (pid ("comboOn"), "Combo FX On", true));
    p.push_back (std::make_unique<AudioParameterBool> (pid ("combo0"), "Scratch + Stop Tape", true));
    p.push_back (std::make_unique<AudioParameterBool> (pid ("combo1"), "Vocal Stutter + Scratch", true));
    p.push_back (std::make_unique<AudioParameterBool> (pid ("combo2"), "Reverse + Deep Stop", true));
    p.push_back (std::make_unique<AudioParameterBool> (pid ("combo3"), "Scratch + Glitch + Restart", true));

    p.push_back (std::make_unique<F> (pid ("bpm"), "BPM", NormalisableRange<float> (40.0f, 220.0f, 0.1f), 90.0f,
        FA().withStringFromValueFunction ([] (float v, int) { return String (v, 1); })));
    p.push_back (std::make_unique<AudioParameterBool> (pid ("hostSync"), "Host Sync", true));

    p.push_back (std::make_unique<F> (pid ("mix"), "Mix", NormalisableRange<float> (0.0f, 1.0f), 1.0f,
        FA().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; })));
    p.push_back (std::make_unique<F> (pid ("gain"), "Gain", NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
        FA().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));

    return { p.begin(), p.end() };
}

//==============================================================================
BlessedScratchProcessor::BlessedScratchProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BLESSED_SCRATCH", createLayout())
{
    seed = (uint32_t) Random::getSystemRandom().nextInt() | 1u;
    capture.setSize (2, (int) (kMaxCaptureSeconds * 44100.0));
    capture.clear();
}

void BlessedScratchProcessor::prepareToPlay (double sr, int)
{
    const double old = currentSampleRate.exchange (sr);
    if (std::abs (old - sr) > 1.0 || capture.getNumSamples() < (int) (kMaxCaptureSeconds * sr))
    {
        SpinLock::ScopedLockType l (captureLock);
        recording = false;
        capture.setSize (2, (int) (kMaxCaptureSeconds * sr));
        capture.clear();
        captureLen = 0;
        captureVersion++;
    }
    mixSm.reset (sr, 0.02);  mixSm.setCurrentAndTargetValue (rawParam ("mix"));
    gainSm.reset (sr, 0.02); gainSm.setCurrentAndTargetValue (Decibels::decibelsToGain (rawParam ("gain")));
}

bool BlessedScratchProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == AudioChannelSet::stereo() || out == AudioChannelSet::mono())
           && out == l.getMainInputChannelSet();
}

//==============================================================================
void BlessedScratchProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    for (int c = getTotalNumInputChannels(); c < numCh; ++c) buffer.clear (c, 0, numSamples);
    if (numCh == 0 || numSamples == 0) return;

    bool hostPlaying = false, havePpq = false;
    double ppq = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) hostBpm.store (*b);
            hostPlaying = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { ppq = *q; havePpq = true; }
        }

    // ---- record the incoming audio -----------------------------------------
    if (recording.load())
    {
        SpinLock::ScopedTryLockType l (captureLock);
        if (l.isLocked())
        {
            const int len = captureLen.load();
            const int cap = capture.getNumSamples();
            const int n = jmin (numSamples, cap - len);
            if (n > 0)
            {
                for (int c = 0; c < 2; ++c)
                    capture.copyFrom (c, len, buffer, jmin (c, numCh - 1), 0, n);
                captureLen.store (len + n);
                captureVersion++;
            }
            if (n < numSamples) recording = false;      // buffer full
        }
    }

    // ---- take a freshly generated pattern, if any ------------------------------
    {
        SpinLock::ScopedTryLockType l (resultLock);
        if (l.isLocked() && newPending)
        {
            std::swap (audioResult, pending);          // old result is released on the GUI thread
            newPending = false;
            previewPos = 0.0;
        }
    }
    if (restartPreview.exchange (false)) previewPos = 0.0;

    mixSm.setTargetValue (rawParam ("mix"));
    gainSm.setTargetValue (Decibels::decibelsToGain (rawParam ("gain")));

    const bool previewOn = previewing.load() && audioResult != nullptr && ! audioResult->audio.empty();

    if (previewOn)
    {
        const auto& A = audioResult->audio;
        const int N = (int) A.size();
        const double sr = getSampleRate();
        double inc = audioResult->sampleRate / sr;
        double pos = previewPos;

        const bool sync = rawParam ("hostSync") > 0.5f && hostPlaying && havePpq
                          && hostBpm.load() > 0.0 && audioResult->beats > 0.0;
        if (sync)
        {
            double beatPos = std::fmod (ppq, audioResult->beats);
            if (beatPos < 0.0) beatPos += audioResult->beats;
            pos = beatPos * (60.0 / audioResult->bpm) * audioResult->sampleRate;
            inc *= hostBpm.load() / audioResult->bpm;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            const float m = mixSm.getNextValue();
            const float g = gainSm.getNextValue();
            int i0 = (int) pos; if (i0 >= N) i0 = 0;
            const int i1 = (i0 + 1) % N;
            const float fr = (float) (pos - (double) (int) pos);
            const float wl = A.ch[0][(size_t) i0] + (A.ch[0][(size_t) i1] - A.ch[0][(size_t) i0]) * fr;
            const float wr = A.ch[1][(size_t) i0] + (A.ch[1][(size_t) i1] - A.ch[1][(size_t) i0]) * fr;

            for (int c = 0; c < numCh; ++c)
            {
                float* d = buffer.getWritePointer (c);
                const float wet = (c == 0) ? wl : wr;
                d[i] = (d[i] * (1.0f - m) + wet * m) * g;
            }
            pos += inc;
            while (pos >= (double) N) pos -= (double) N;
        }
        previewPos = pos;
        previewPhase.store ((float) (pos / (double) N));
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = gainSm.getNextValue();
            for (int c = 0; c < numCh; ++c) buffer.getWritePointer (c)[i] *= g;
        }
    }

    const float pl = buffer.getMagnitude (0, 0, numSamples);
    const float pr = numCh > 1 ? buffer.getMagnitude (1, 0, numSamples) : pl;
    levelL.store (jmax (pl, levelL.load() * 0.9f));
    levelR.store (jmax (pr, levelR.load() * 0.9f));
}

//==============================================================================
void BlessedScratchProcessor::setRecording (bool on)
{
    if (on)
    {
        SpinLock::ScopedLockType l (captureLock);
        captureLen = 0;
        capture.clear();
        captureVersion++;
        selStart = 0.0f; selEnd = 1.0f;
    }
    recording = on;
}

void BlessedScratchProcessor::clearCapture()
{
    recording = false;
    SpinLock::ScopedLockType l (captureLock);
    captureLen = 0;
    capture.clear();
    captureVersion++;
    selStart = 0.0f; selEnd = 1.0f;
}

bool BlessedScratchProcessor::loadAudioFile (const File& file)
{
    AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples < 64) return false;

    const double sr = currentSampleRate.load();
    const int64 maxIn = (int64) (kMaxCaptureSeconds * reader->sampleRate);
    const int inLen = (int) jmin (reader->lengthInSamples, maxIn);

    AudioBuffer<float> in (2, inLen);
    reader->read (&in, 0, inLen, 0, true, true);
    if (reader->numChannels < 2) in.copyFrom (1, 0, in, 0, 0, inLen);

    AudioBuffer<float> out;
    if (std::abs (reader->sampleRate - sr) > 0.5)
    {
        const double ratio = reader->sampleRate / sr;
        const int outLen = jmin ((int) (kMaxCaptureSeconds * sr), (int) ((double) inLen / ratio));
        out.setSize (2, outLen);
        for (int c = 0; c < 2; ++c)
        {
            LagrangeInterpolator interp;
            interp.process (ratio, in.getReadPointer (c), out.getWritePointer (c), outLen);
        }
    }
    else out = std::move (in);

    recording = false;
    SpinLock::ScopedLockType l (captureLock);
    const int n = jmin (out.getNumSamples(), capture.getNumSamples());
    capture.clear();
    for (int c = 0; c < 2; ++c) capture.copyFrom (c, 0, out, c, 0, n);
    captureLen = n;
    selStart = 0.0f; selEnd = 1.0f;
    captureVersion++;
    return n > 0;
}

void BlessedScratchProcessor::getPeaks (std::vector<float>& peaks, int numBars)
{
    peaks.assign ((size_t) jmax (1, numBars), 0.0f);
    SpinLock::ScopedLockType l (captureLock);
    const int len = captureLen.load();
    if (len <= 0) return;

    const double per = (double) len / (double) peaks.size();
    const float* a = capture.getReadPointer (0);
    const float* b = capture.getReadPointer (1);
    for (size_t i = 0; i < peaks.size(); ++i)
    {
        const int s0 = (int) ((double) i * per);
        const int s1 = jmin (len, jmax (s0 + 1, (int) ((double) (i + 1) * per)));
        const int stride = jmax (1, (s1 - s0) / 48);
        float m = 0.0f;
        for (int s = s0; s < s1; s += stride) m = jmax (m, std::abs (a[s]), std::abs (b[s]));
        peaks[i] = m;
    }
}

//==============================================================================
bs::Settings BlessedScratchProcessor::buildSettings() const
{
    bs::Settings S;
    S.mode          = (bs::Mode) roundToInt (rawParam ("mode"));
    const double hb = hostBpm.load();
    S.bpm           = (rawParam ("hostSync") > 0.5f && hb > 0.0) ? hb : (double) rawParam ("bpm");
    S.durationBeats = kDurationBeats[jlimit (0, 4, roundToInt (rawParam ("duration")))];
    S.stepsPerBeat  = kStepsPerBeat [jlimit (0, 5, roundToInt (rawParam ("rhythm")))];
    S.speed         = rawParam ("speed");
    S.complexity    = rawParam ("complexity");
    S.intensity     = rawParam ("intensity");
    S.cuts          = kCuts[jlimit (0, 10, roundToInt (rawParam ("cuts")))];
    S.variation     = (bs::Variation) roundToInt (rawParam ("variation"));
    S.pitchSemis    = roundToInt (rawParam ("pitch")) - 12;
    S.stopType      = (bs::StopType) roundToInt (rawParam ("stopType"));
    S.stopTime      = rawParam ("stopTime");
    S.pitchDrop     = rawParam ("pitchDrop");
    S.curve         = rawParam ("curve");
    S.texture       = (bs::Texture) roundToInt (rawParam ("texture"));
    S.restart       = rawParam ("restart") > 0.5f;

    if (rawParam ("comboOn") > 0.5f)
    {
        S.combos = { { rawParam ("combo0") > 0.5f, rawParam ("combo1") > 0.5f,
                       rawParam ("combo2") > 0.5f, rawParam ("combo3") > 0.5f } };
    }
    else
    {
        S.combos = { { true, true, true, true } };
    }
    S.seed = seed;
    return S;
}

bool BlessedScratchProcessor::generate (bool newSeed)
{
    bs::Audio src;
    {
        SpinLock::ScopedLockType l (captureLock);
        const int len = captureLen.load();
        if (len < 256) return false;

        int rs = (int) (jlimit (0.0f, 1.0f, selStart.load()) * (float) len);
        int re = (int) (jlimit (0.0f, 1.0f, selEnd.load())   * (float) len);
        if (re - rs < 256) { rs = 0; re = len; }

        src.resize ((size_t) (re - rs));
        for (int c = 0; c < 2; ++c)
            std::copy (capture.getReadPointer (c) + rs, capture.getReadPointer (c) + re, src.ch[(size_t) c].begin());
    }

    if (newSeed) seed = (uint32_t) Random::getSystemRandom().nextInt() | 1u;

    const bs::Settings S = buildSettings();
    const double sr = currentSampleRate.load();

    auto pat = std::make_shared<RenderedPattern>();
    pat->audio = bs::render (src, 0, src.size(), S, sr);
    pat->beats = S.durationBeats;
    pat->bpm = S.bpm;
    pat->sampleRate = sr;

    {
        SpinLock::ScopedLockType l (resultLock);
        pending = pat;
        displayResult = pat;
        newPending = true;
    }
    resultVersion++;
    return true;
}

std::shared_ptr<const RenderedPattern> BlessedScratchProcessor::getDisplayResult()
{
    SpinLock::ScopedLockType l (resultLock);
    return displayResult;
}

void BlessedScratchProcessor::setPreviewing (bool on)
{
    if (on) restartPreview = true;
    previewing = on;
}

bool BlessedScratchProcessor::exportWav (const File& file)
{
    auto pat = getDisplayResult();
    if (pat == nullptr || pat->audio.empty()) return false;

    const int n = (int) pat->audio.size();
    AudioBuffer<float> b (2, n);
    for (int c = 0; c < 2; ++c)
        std::copy (pat->audio.ch[(size_t) c].begin(), pat->audio.ch[(size_t) c].end(), b.getWritePointer (c));

    file.deleteFile();
    std::unique_ptr<FileOutputStream> os (file.createOutputStream());
    if (os == nullptr) return false;

    WavAudioFormat wav;
    std::unique_ptr<AudioFormatWriter> writer (wav.createWriterFor (os.get(), pat->sampleRate, 2, 24, {}, 0));
    if (writer == nullptr) return false;
    os.release();                                   // the writer owns the stream now
    return writer->writeFromAudioSampleBuffer (b, 0, n);
}

//==============================================================================
void BlessedScratchProcessor::toggleAB()
{
    abSlot[abCurrent] = apvts.copyState();
    abCurrent = 1 - abCurrent;
    if (abSlot[abCurrent].isValid()) apvts.replaceState (abSlot[abCurrent].createCopy());
    else                              abSlot[abCurrent] = apvts.copyState();
}

//==============================================================================
int BlessedScratchProcessor::getNumFactoryPresets() { return (int) (sizeof (kFactory) / sizeof (kFactory[0])); }

String BlessedScratchProcessor::getFactoryPresetName (int i)
{
    return String::fromUTF8 (kFactory[jlimit (0, getNumFactoryPresets() - 1, i)].name);
}

void BlessedScratchProcessor::setParamPlain (const String& id, float v)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

void BlessedScratchProcessor::applyFactoryPreset (int i)
{
    const auto& f = kFactory[jlimit (0, getNumFactoryPresets() - 1, i)];
    setParamPlain ("speed", f.speed);
    setParamPlain ("complexity", f.complexity);
    setParamPlain ("intensity", f.intensity);
    setParamPlain ("duration", (float) f.duration);
    setParamPlain ("rhythm", (float) f.rhythm);
    setParamPlain ("cuts", (float) f.cuts);
    setParamPlain ("variation", (float) f.variation);
    setParamPlain ("pitch", (float) (f.pitch + 12));
    setParamPlain ("stopType", (float) f.stopType);
    setParamPlain ("stopTime", f.stopTime);
    setParamPlain ("pitchDrop", f.pitchDrop);
    setParamPlain ("curve", f.curve);
    setParamPlain ("texture", (float) f.texture);
}

File BlessedScratchProcessor::getUserPresetDir() const
{
    return File::getSpecialLocation (File::userApplicationDataDirectory)
               .getChildFile ("BlessedScratch").getChildFile ("Presets");
}

StringArray BlessedScratchProcessor::getUserPresetNames() const
{
    StringArray names;
    for (auto& f : getUserPresetDir().findChildFiles (File::findFiles, false, "*.bspreset"))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool BlessedScratchProcessor::saveUserPreset (const String& name)
{
    const auto clean = File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto dir = getUserPresetDir();
    if (! dir.createDirectory().wasOk()) return false;
    if (auto xml = apvts.copyState().createXml())
        return xml->writeTo (dir.getChildFile (clean + ".bspreset"));
    return false;
}

bool BlessedScratchProcessor::loadUserPreset (const String& name)
{
    if (auto xml = parseXML (getUserPresetDir().getChildFile (name + ".bspreset")))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (ValueTree::fromXml (*xml));
            return true;
        }
    return false;
}

//==============================================================================
void BlessedScratchProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("selStart", selStart.load(), nullptr);
    state.setProperty ("selEnd", selEnd.load(), nullptr);
    state.setProperty ("seed", (int) seed, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void BlessedScratchProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = ValueTree::fromXml (*xml);
            selStart = (float) (double) vt.getProperty ("selStart", 0.0);
            selEnd   = (float) (double) vt.getProperty ("selEnd", 1.0);
            if (vt.hasProperty ("seed")) seed = (uint32_t) (int) vt.getProperty ("seed") | 1u;
            apvts.replaceState (vt);
        }
}

AudioProcessorEditor* BlessedScratchProcessor::createEditor() { return new BlessedScratchEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BlessedScratchProcessor(); }

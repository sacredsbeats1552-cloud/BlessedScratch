#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>
#include "ScratchEngine.h"

using namespace juce;

struct RenderedPattern
{
    bs::Audio audio;
    double beats = 0.0, bpm = 90.0, sampleRate = 44100.0;
};

class BlessedScratchProcessor : public AudioProcessor
{
public:
    static constexpr double kMaxCaptureSeconds = 60.0;

    BlessedScratchProcessor();
    ~BlessedScratchProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "BLESSED SCRATCH"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const String getProgramName (int) override { return {}; }
    void changeProgramName (int, const String&) override {}

    void getStateInformation (MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    //==========================================================================
    static AudioProcessorValueTreeState::ParameterLayout createLayout();
    AudioProcessorValueTreeState apvts;

    // --- capture (source audio) ---
    void setRecording (bool shouldRecord);
    bool isRecording() const noexcept            { return recording.load(); }
    void clearCapture();
    bool loadAudioFile (const File& file);
    int  getCaptureLength() const noexcept       { return captureLen.load(); }
    void getPeaks (std::vector<float>& peaks, int numBars);

    std::atomic<float>    selStart { 0.0f }, selEnd { 1.0f };      // normalised region
    std::atomic<uint32_t> captureVersion { 0 }, resultVersion { 0 };

    // --- pattern generation ---
    bool generate (bool newSeed);
    std::shared_ptr<const RenderedPattern> getDisplayResult();
    bool exportWav (const File& file);
    bs::Settings buildSettings() const;

    void  setPreviewing (bool on);
    bool  isPreviewing() const noexcept          { return previewing.load(); }
    float getPreviewPhase() const noexcept       { return previewPhase.load(); }

    // --- A/B ---
    void toggleAB();
    int  getAB() const noexcept                  { return abCurrent; }

    // --- presets ---
    static int  getNumFactoryPresets();
    static String getFactoryPresetName (int index);
    void applyFactoryPreset (int index);
    File getUserPresetDir() const;
    StringArray getUserPresetNames() const;
    bool saveUserPreset (const String& name);
    bool loadUserPreset (const String& name);

    void setParamPlain (const String& id, float plainValue);

    std::atomic<float> levelL { 0.0f }, levelR { 0.0f };
    std::atomic<double> hostBpm { 0.0 };

private:
    float rawParam (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    // capture
    AudioBuffer<float>   capture;
    SpinLock             captureLock;
    std::atomic<int>     captureLen { 0 };
    std::atomic<bool>    recording { false };

    // result hand-over (GUI -> audio thread, allocation free on the audio side)
    SpinLock                                 resultLock;
    std::shared_ptr<const RenderedPattern>   pending, displayResult;
    bool                                     newPending = false;
    std::shared_ptr<const RenderedPattern>   audioResult;     // audio thread only

    std::atomic<bool>  previewing { false };
    std::atomic<float> previewPhase { 0.0f };
    double             previewPos = 0.0;                      // audio thread only
    std::atomic<bool>  restartPreview { false };

    SmoothedValue<float> mixSm, gainSm;
    std::atomic<double>  currentSampleRate { 44100.0 };
    uint32_t             seed = 0;

    ValueTree abSlot[2];
    int       abCurrent = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlessedScratchProcessor)
};

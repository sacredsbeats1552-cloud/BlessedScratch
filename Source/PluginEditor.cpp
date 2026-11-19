#include "PluginEditor.h"
#include "UIComponents.h"

using namespace bsui;

namespace
{
constexpr int kW = 1600, kH = 900;
}

//==============================================================================
class BlessedScratchEditor::MainPanel : public Component, private Timer
{
public:
    explicit MainPanel (BlessedScratchProcessor& p)
        : proc (p), apvts (p.apvts),
          waveform (p), meter (p),
          tabScratch ("Scratch", Icon::Vinyl), tabStop ("Stop Tape", Icon::Cassette), tabCombo ("Combo FX", Icon::Crown),
          btnGenerate ("Generate", Icon::Play, true, 30.0f),
          btnRegen ("Regenerate", Icon::Refresh),
          btnPreview ("Preview", Icon::Headphones),
          btnAB ("A/B", Icon::Stop),
          btnRec ("REC", Icon::Rec, false, 20.0f), btnLoad ("Load", Icon::Load, false, 20.0f),
          btnClear ("Clear", Icon::Clear, false, 20.0f), btnView ("View: Source", Icon::View, false, 20.0f),
          btnExport ("Export WAV", Icon::Export, false, 20.0f),
          knobSpeed (apvts, "speed", "Speed"), knobComplexity (apvts, "complexity", "Complexity"),
          knobIntensity (apvts, "intensity", "Intensity"), knobDuration (apvts, "duration", "Duration"),
          cbRhythm (apvts, "rhythm", "Rhythm", Icon::Note), cbCuts (apvts, "cuts", "Cuts", Icon::Cuts),
          cbVariation (apvts, "variation", "Variation", Icon::Dice), cbPitch (apvts, "pitch", "Pitch", Icon::Pitch),
          stopTiles (apvts),
          knobStopTime (apvts, "stopTime", "Stop Time"), knobPitchDrop (apvts, "pitchDrop", "Pitch Drop"),
          knobTexture (apvts, "texture", "Texture"),
          curveView (apvts),
          knobMix (apvts, "mix", "Mix"), knobGain (apvts, "gain", "Gain"),
          chk0 ("Scratch + Stop Tape"), chk1 ("Vocal Stutter + Scratch"),
          chk2 ("Reverse + Deep Stop"), chk3 ("Scratch + Glitch + Restart")
    {
        logo = ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);
        setLookAndFeel (&laf);

        for (auto* c : std::initializer_list<Component*> { &waveform, &meter, &tabScratch, &tabStop, &tabCombo,
                 &btnGenerate, &btnRegen, &btnPreview, &btnAB, &btnRec, &btnLoad, &btnClear, &btnView, &btnExport,
                 &presets, &knobSpeed, &knobComplexity, &knobIntensity, &knobDuration,
                 &cbRhythm, &cbCuts, &cbVariation, &cbPitch,
                 &stopTiles, &knobStopTime, &knobPitchDrop, &knobTexture, &curveView, &knobMix, &knobGain,
                 &comboSwitch, &chk0, &chk1, &chk2, &chk3, &hostToggle, &bpmSlider })
            addAndMakeVisible (c);

        // --- BPM box + host sync
        bpmSlider.setSliderStyle (Slider::LinearBar);
        bpmSlider.setTextBoxIsEditable (true);
        bpmSlider.setTextBoxStyle (Slider::TextBoxLeft, false, 150, 50);
        bpmAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (apvts, "bpm", bpmSlider);
        bpmSlider.setDoubleClickReturnValue (false, 90.0);
        hostAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (apvts, "hostSync", hostToggle);

        // --- combo fx
        comboAtt  = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (apvts, "comboOn", comboSwitch);
        c0Att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (apvts, "combo0", chk0);
        c1Att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (apvts, "combo1", chk1);
        c2Att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (apvts, "combo2", chk2);
        c3Att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (apvts, "combo3", chk3);

        // --- tabs <-> mode parameter
        modeAtt = std::make_unique<ParameterAttachment> (*apvts.getParameter ("mode"), [this] (float v)
        {
            mode = roundToInt (v);
            tabScratch.setToggleState (mode == 0, dontSendNotification);
            tabStop.setToggleState (mode == 1, dontSendNotification);
            tabCombo.setToggleState (mode == 2, dontSendNotification);
            updateModeVisuals();
            repaint();
        });
        tabScratch.onClick = [this] { modeAtt->setValueAsCompleteGesture (0.0f); };
        tabStop.onClick    = [this] { modeAtt->setValueAsCompleteGesture (1.0f); };
        tabCombo.onClick   = [this] { modeAtt->setValueAsCompleteGesture (2.0f); };
        modeAtt->sendInitialUpdate();

        // --- actions
        btnGenerate.onClick = [this] { doGenerate (false, true); };
        btnRegen.onClick    = [this] { doGenerate (true, true); };
        btnPreview.setClickingTogglesState (true);
        btnPreview.onClick  = [this]
        {
            const bool want = btnPreview.getToggleState();
            if (want && proc.getDisplayResult() == nullptr && ! doGenerate (false, false))
            {
                btnPreview.setToggleState (false, dontSendNotification);
                return;
            }
            proc.setPreviewing (want);
        };
        btnAB.abIndex = [this] { return proc.getAB(); };
        btnAB.onClick = [this] { proc.toggleAB(); repaint(); };

        btnRec.setClickingTogglesState (true);
        btnRec.danger = true;
        btnRec.onClick = [this]
        {
            proc.setRecording (btnRec.getToggleState());
            if (btnRec.getToggleState()) waveform.setShowResult (false), btnView.setButtonText ("View: Source");
        };
        btnLoad.onClick = [this] { chooseFile(); };
        btnClear.onClick = [this] { proc.clearCapture(); btnRec.setToggleState (false, dontSendNotification); };
        btnView.onClick = [this]
        {
            waveform.setShowResult (! waveform.getShowResult());
            btnView.setButtonText (waveform.getShowResult() ? "View: Result" : "View: Source");
        };
        btnExport.onClick = [this] { doExport(); };

        presets.onSelect = [this] (int i) { onPreset (i); };
        presets.selected = 0;

        startTimerHz (30);
    }

    ~MainPanel() override { setLookAndFeel (nullptr); }

    //--------------------------------------------------------------------------
    void resized() override
    {
        waveform.setBounds (590, 135, 970, 160);
        tabScratch.setBounds (680, 20, 235, 78);
        tabStop.setBounds (903, 20, 245, 78);
        tabCombo.setBounds (1135, 20, 250, 78);
        bpmSlider.setBounds (1410, 28, 170, 52);
        hostToggle.setBounds (1420, 82, 150, 24);

        btnView.setBounds (1010, 100, 170, 30);
        btnRec.setBounds (1188, 100, 82, 30);
        btnLoad.setBounds (1276, 100, 90, 30);
        btnClear.setBounds (1372, 100, 92, 30);
        btnExport.setBounds (1470, 100, 112, 30);

        presets.setBounds (24, 340, 262, 300);

        btnGenerate.setBounds (322, 306, 285, 66);
        btnRegen.setBounds (625, 312, 235, 54);
        btnPreview.setBounds (876, 312, 195, 54);
        btnAB.setBounds (1082, 312, 150, 54);

        const int kx[4] = { 368, 570, 780, 990 };
        KnobCell* ks[4] = { &knobSpeed, &knobComplexity, &knobIntensity, &knobDuration };
        for (int i = 0; i < 4; ++i) ks[i]->setBounds (kx[i], 424, 140, 160);
        ComboCell* cs[4] = { &cbRhythm, &cbCuts, &cbVariation, &cbPitch };
        for (int i = 0; i < 4; ++i) cs[i]->setBounds (kx[i] - 14, 586, 168, 64);

        comboSwitch.setBounds (1490, 348, 68, 38);
        chk0.setBounds (1250, 412, 320, 44);
        chk1.setBounds (1250, 464, 320, 44);
        chk2.setBounds (1250, 516, 320, 44);
        chk3.setBounds (1250, 568, 320, 44);

        stopTiles.setBounds (34, 722, 670, 112);
        knobStopTime.setBounds (762, 676, 100, 160);
        knobPitchDrop.setBounds (872, 676, 110, 160);
        knobTexture.setBounds (990, 676, 100, 160);
        curveView.setBounds (1128, 690, 200, 140);
        knobMix.setBounds (1362, 684, 90, 150);
        knobGain.setBounds (1450, 684, 90, 150);
        meter.setBounds (1550, 708, 22, 112);
    }

    void paint (Graphics& g) override
    {
        // background
        g.fillAll (Col::bg0);
        g.setGradientFill (ColourGradient (Colour (0xff1c0c34), 800.0f, 260.0f, Col::bg0, 0.0f, 900.0f, true));
        g.fillRect (getLocalBounds());
        g.setGradientFill (ColourGradient (Col::violet.withAlpha (0.10f), 1500.0f, 120.0f, Colours::transparentBlack, 1100.0f, 420.0f, true));
        g.fillRect (getLocalBounds());
        g.setColour (Col::violet.withAlpha (0.8f));
        g.drawRect (getLocalBounds().toFloat(), 2.0f);

        if (logo.isValid()) g.drawImageAt (logo, 0, 0);

        const bool aS = mode == 0, aT = mode == 1, aC = mode == 2;

        // PRESETS
        drawPanel (g, { 14.0f, 285.0f, 282.0f, 368.0f }, true);
        drawIcon (g, Icon::Crown, { 32.0f, 300.0f, 34.0f, 28.0f }, Col::violetBright);
        g.setColour (Col::lavender); g.setFont (Fonts::heading (30.0f));
        g.drawText ("PRESETS", 76, 296, 200, 36, Justification::centredLeft);

        // RHYTHM & CONTROL
        drawPanel (g, { 310.0f, 390.0f, 890.0f, 262.0f }, aS);
        drawIcon (g, Icon::Bars, { 330.0f, 402.0f, 32.0f, 28.0f }, Col::violetBright);
        g.setColour (Col::lavender); g.setFont (Fonts::heading (28.0f));
        g.drawText ("RHYTHM & CONTROL", 372, 398, 360, 36, Justification::centredLeft);

        // COMBO FX
        drawPanel (g, { 1226.0f, 322.0f, 358.0f, 330.0f }, aC);
        drawIcon (g, Icon::Star, { 1248.0f, 346.0f, 36.0f, 36.0f }, Col::violetBright);
        g.setColour (Col::white); g.setFont (Fonts::heading (34.0f));
        g.drawText ("COMBO FX", 1296, 344, 190, 42, Justification::centredLeft);

        // STOP TAPE
        drawPanel (g, { 14.0f, 664.0f, 716.0f, 190.0f }, aT);
        drawIcon (g, Icon::Cassette, { 38.0f, 676.0f, 48.0f, 38.0f }, Col::lavender, 2.0f);
        g.setColour (Col::white); g.setFont (Fonts::heading (36.0f));
        g.drawText ("STOP TAPE", 98, 672, 260, 46, Justification::centredLeft);
        drawPanel (g, { 744.0f, 664.0f, 372.0f, 190.0f }, aT);
        drawPanel (g, { 1122.0f, 664.0f, 214.0f, 190.0f }, aT);

        // OUTPUT
        drawPanel (g, { 1344.0f, 658.0f, 240.0f, 196.0f }, true);
        drawIcon (g, Icon::Bars, { 1362.0f, 664.0f, 22.0f, 20.0f }, Col::violetBright);
        g.setColour (Col::white); g.setFont (Fonts::heading (22.0f));
        g.drawText ("OUTPUT", 1390, 662, 140, 26, Justification::centredLeft);

        // tag + footer
        {
            Graphics::ScopedSaveState ss (g);
            g.addTransform (AffineTransform::rotation (-0.07f, 170.0f, 650.0f));
            g.setColour (Col::violetBright); g.setFont (Fonts::heading (22.0f));
            g.drawText ("SCRATCH LIKE A PRO", 92, 628, 250, 28, Justification::centredLeft);
        }
        g.setColour (Col::dim); g.setFont (Fonts::labelBold (20.0f));
        g.drawText (String ("B L E S S E D   S C R A T C H        v") + JucePlugin_VersionString, 38, 862, 520, 28, Justification::centredLeft);
        g.setColour (Col::violetBright); g.setFont (Fonts::heading (24.0f));
        g.drawText (String::fromUTF8 ("GOOD VIBES  \xe2\x80\xa2  GREAT FX"), 1100, 858, 440, 32, Justification::centredRight);
        drawIcon (g, Icon::Crown, { 1550.0f, 862.0f, 30.0f, 24.0f }, Col::violetBright);

        // BPM caption
        g.setColour (Col::dim); g.setFont (Fonts::labelBold (20.0f));
        g.drawText ("BPM", 1410, 4, 80, 24, Justification::centredLeft);
    }

private:
    //--------------------------------------------------------------------------
    void timerCallback() override
    {
        meter.tick();
        if (btnPreview.getToggleState() != proc.isPreviewing()) btnPreview.setToggleState (proc.isPreviewing(), dontSendNotification);
        if (btnRec.getToggleState() != proc.isRecording())
        {
            btnRec.setToggleState (proc.isRecording(), dontSendNotification);     // buffer full -> auto stop
        }
        if (lastAB != proc.getAB()) { lastAB = proc.getAB(); btnAB.repaint(); }
    }

    bool doGenerate (bool newSeed, bool autoPreview)
    {
        if (proc.isRecording()) { proc.setRecording (false); btnRec.setToggleState (false, dontSendNotification); }
        if (! proc.generate (newSeed))
        {
            waveform.showMessage ("Nothing to process yet: press REC and play audio, or LOAD / drop an audio file");
            return false;
        }
        waveform.setShowResult (true);
        btnView.setButtonText ("View: Result");
        if (autoPreview)
        {
            proc.setPreviewing (true);
            btnPreview.setToggleState (true, dontSendNotification);
        }
        return true;
    }

    void chooseFile()
    {
        chooser = std::make_unique<FileChooser> ("Load audio", File::getSpecialLocation (File::userMusicDirectory),
                                                 "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
            [this] (const FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (f.existsAsFile())
                {
                    const bool ok = proc.loadAudioFile (f);
                    waveform.setShowResult (false);
                    btnView.setButtonText ("View: Source");
                    waveform.showMessage (ok ? "Audio loaded" : "Could not read that file");
                }
            });
    }

    void doExport()
    {
        if (proc.getDisplayResult() == nullptr && ! doGenerate (false, false)) return;
        chooser = std::make_unique<FileChooser> ("Export WAV", File::getSpecialLocation (File::userDocumentsDirectory)
                                                                   .getChildFile ("BlessedScratch_pattern.wav"), "*.wav");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
            [this] (const FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f == File()) return;
                if (! f.hasFileExtension ("wav")) f = f.withFileExtension ("wav");
                waveform.showMessage (proc.exportWav (f) ? "Exported: " + f.getFileName() : "Export failed");
            });
    }

    void onPreset (int i)
    {
        if (i < BlessedScratchProcessor::getNumFactoryPresets())
        {
            proc.applyFactoryPreset (i);
            return;
        }

        PopupMenu m;
        m.addItem (1, "Save current settings as...");
        const auto names = proc.getUserPresetNames();
        if (! names.isEmpty()) m.addSeparator();
        for (int k = 0; k < names.size(); ++k) m.addItem (100 + k, names[k]);

        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&presets).withMinimumWidth (240),
            [this, names] (int r)
            {
                if (r == 1)
                {
                    auto* aw = new AlertWindow ("Save preset", "Preset name:", MessageBoxIconType::NoIcon);
                    aw->addTextEditor ("name", "My Preset");
                    aw->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
                    aw->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
                    aw->enterModalState (true, ModalCallbackFunction::create ([this, aw] (int res)
                    {
                        if (res == 1) waveform.showMessage (proc.saveUserPreset (aw->getTextEditorContents ("name")) ? "Preset saved" : "Could not save preset");
                    }), true);
                }
                else if (r >= 100 && r - 100 < names.size())
                    proc.loadUserPreset (names[r - 100]);
            });
    }

    void updateModeVisuals()
    {
        auto setA = [] (std::initializer_list<Component*> cs, bool on) { for (auto* c : cs) c->setAlpha (on ? 1.0f : 0.5f); };
        setA ({ &knobSpeed, &knobComplexity, &knobIntensity, &knobDuration, &cbRhythm, &cbCuts, &cbVariation, &cbPitch }, mode == 0);
        setA ({ &stopTiles, &knobStopTime, &knobPitchDrop, &knobTexture, &curveView }, mode == 1);
        setA ({ &comboSwitch, &chk0, &chk1, &chk2, &chk3 }, mode == 2);
    }

    //--------------------------------------------------------------------------
    BlessedScratchProcessor& proc;
    AudioProcessorValueTreeState& apvts;
    BSLookAndFeel laf;
    Image logo;
    int mode = 0, lastAB = -1;

    WaveformView waveform;
    LevelMeter meter;
    TabButton tabScratch, tabStop, tabCombo;
    NeonButton btnGenerate, btnRegen, btnPreview, btnAB, btnRec, btnLoad, btnClear, btnView, btnExport;
    PresetList presets;
    KnobCell knobSpeed, knobComplexity, knobIntensity, knobDuration;
    ComboCell cbRhythm, cbCuts, cbVariation, cbPitch;
    StopTiles stopTiles;
    KnobCell knobStopTime, knobPitchDrop, knobTexture;
    CurveView curveView;
    KnobCell knobMix, knobGain;
    SwitchButton comboSwitch;
    CheckRow chk0, chk1, chk2, chk3;
    HostToggle hostToggle;
    Slider bpmSlider;

    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> bpmAtt;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> hostAtt, comboAtt, c0Att, c1Att, c2Att, c3Att;
    std::unique_ptr<ParameterAttachment> modeAtt;
    std::unique_ptr<FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};

//==============================================================================
BlessedScratchEditor::BlessedScratchEditor (BlessedScratchProcessor& p)
    : AudioProcessorEditor (&p)
{
    panel = std::make_unique<MainPanel> (p);
    addAndMakeVisible (*panel);

    setResizable (true, true);
    setResizeLimits (800, 450, 2400, 1350);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (1200, 675);
}

BlessedScratchEditor::~BlessedScratchEditor() = default;

void BlessedScratchEditor::paint (Graphics& g) { g.fillAll (Colours::black); }

void BlessedScratchEditor::resized()
{
    panel->setBounds (0, 0, kW, kH);
    panel->setTransform (AffineTransform::scale ((float) getWidth() / (float) kW));
}

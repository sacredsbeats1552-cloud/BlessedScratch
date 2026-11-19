#pragma once
#include "PluginProcessor.h"

class BlessedScratchEditor : public AudioProcessorEditor
{
public:
    explicit BlessedScratchEditor (BlessedScratchProcessor&);
    ~BlessedScratchEditor() override;

    void resized() override;
    void paint (Graphics&) override;

private:
    class MainPanel;
    std::unique_ptr<MainPanel> panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlessedScratchEditor)
};

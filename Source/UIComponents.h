#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "UIBase.h"
#include "PluginProcessor.h"

namespace bsui
{
using namespace juce;

//==============================================================================
// Big chamfered action button with icon (GENERATE, REGENERATE, PREVIEW, A/B ...)
class NeonButton : public Button
{
public:
    NeonButton (const String& label, Icon i, bool primaryStyle = false, float fontSize = 26.0f)
        : Button (label), icon (i), primary (primaryStyle), fontH (fontSize) {}

    std::function<int()> abIndex;           // if set, draws the "A / B" selector
    bool danger = false;                    // red accent (REC)

    void paintButton (Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        const bool on = getToggleState();
        auto path = chamferedRect (r, jmin (12.0f, r.getHeight() * 0.3f));
        const Colour accent = danger ? Col::red : Col::violetBright;

        if (primary || on)
        {
            g.setColour (accent.withAlpha (0.30f));
            g.strokePath (path, PathStrokeType (8.0f));
            g.setGradientFill (ColourGradient (Col::violetBright.brighter (0.1f), 0, r.getY(), Col::violetDark.brighter (0.2f), 0, r.getBottom(), false));
            if (danger) g.setGradientFill (ColourGradient (Col::red, 0, r.getY(), Colour (0xff6a0f26), 0, r.getBottom(), false));
        }
        else
        {
            g.setGradientFill (ColourGradient (Colour (0xff1c1530), 0, r.getY(), Colour (0xff0a0712), 0, r.getBottom(), false));
        }
        g.fillPath (path);
        g.setColour ((primary || on) ? Colours::white.withAlpha (0.85f) : (over ? Col::violetBright : Col::violet.withAlpha (0.75f)));
        g.strokePath (path, PathStrokeType (down ? 2.4f : 1.6f));

        const auto textCol = Colours::white;
        const float iconS = jmin (r.getHeight() * 0.5f, 30.0f);

        if (abIndex)
        {
            auto f = Fonts::labelBold (fontH);
            g.setFont (f);
            const int cur = abIndex();
            const float wA = textWidth (f, "A"), wS = textWidth (f, " / "), wB = textWidth (f, "B");
            const float total = iconS + 10.0f + wA + wS + wB;
            float x0 = r.getCentreX() - total * 0.5f;
            {
                const Rectangle<float> disc (x0 + 1.0f, r.getCentreY() - iconS * 0.45f, iconS * 0.9f, iconS * 0.9f);
                g.setColour (Colours::white);
                g.drawEllipse (disc, 2.0f);
                Path half;
                half.addPieSegment (disc, MathConstants<float>::pi, MathConstants<float>::twoPi, 0.0f);
                g.fillPath (half);
            }
            x0 += iconS + 10.0f;
            g.setColour (cur == 0 ? textCol : Col::dim);   g.drawText ("A", Rectangle<float> (x0, r.getY(), wA, r.getHeight()), Justification::centred);
            g.setColour (Col::dim);                        g.drawText (" / ", Rectangle<float> (x0 + wA, r.getY(), wS, r.getHeight()), Justification::centred);
            g.setColour (cur == 1 ? textCol : Col::dim);   g.drawText ("B", Rectangle<float> (x0 + wA + wS, r.getY(), wB, r.getHeight()), Justification::centred);
            return;
        }

        auto f = Fonts::labelBold (fontH);
        g.setFont (f);
        const String t = getButtonText().toUpperCase();
        const float tw = textWidth (f, t);
        const bool hasIcon = true;
        const float total = (hasIcon ? iconS + 10.0f : 0.0f) + tw;
        float x0 = r.getCentreX() - total * 0.5f;
        if (hasIcon)
        {
            drawIcon (g, icon, { x0, r.getCentreY() - iconS * 0.5f, iconS, iconS }, (primary || on) ? Colours::white : Col::lavender);
            x0 += iconS + 10.0f;
        }
        g.setColour (textCol);
        g.drawText (t, Rectangle<float> (x0, r.getY(), tw + 2.0f, r.getHeight()), Justification::centredLeft);
    }

private:
    Icon  icon;
    bool  primary;
    float fontH;
};

//==============================================================================
// Section tabs: SCRATCH / STOP TAPE / COMBO FX
class TabButton : public Button
{
public:
    TabButton (const String& label, Icon i) : Button (label), icon (i) {}

    void paintButton (Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (3.0f);
        const bool on = getToggleState();
        const float sk = 16.0f;
        Path p;
        p.startNewSubPath (r.getX() + sk, r.getY());
        p.lineTo (r.getRight(), r.getY());
        p.lineTo (r.getRight() - sk, r.getBottom());
        p.lineTo (r.getX(), r.getBottom());
        p.closeSubPath();
        p = p.createPathWithRoundedCorners (6.0f);

        if (on)
        {
            g.setColour (Col::violetBright.withAlpha (0.35f));
            g.strokePath (p, PathStrokeType (10.0f));
            g.setGradientFill (ColourGradient (Col::violetBright, 0, r.getY(), Col::violetDark, 0, r.getBottom(), false));
        }
        else
            g.setGradientFill (ColourGradient (Colour (0xff1a1230), 0, r.getY(), Colour (0xff09060f), 0, r.getBottom(), false));
        g.fillPath (p);
        g.setColour (on ? Colours::white.withAlpha (0.9f) : (over ? Col::violetBright : Col::violet.withAlpha (0.6f)));
        g.strokePath (p, PathStrokeType (1.6f));

        auto f = Fonts::heading (30.0f);
        g.setFont (f);
        const String t = getButtonText();
        const float iconS = 40.0f;
        const float total = iconS + 12.0f + textWidth (f, t);
        float x0 = r.getCentreX() - total * 0.5f + 4.0f;
        drawIcon (g, icon, { x0, r.getCentreY() - iconS * 0.5f, iconS, iconS }, on ? Colours::white : Col::lavender, 2.2f);
        g.setColour (on ? Colours::white : Col::lavender);
        g.drawText (t, Rectangle<float> (x0 + iconS + 12.0f, r.getY(), textWidth (f, t) + 4.0f, r.getHeight()), Justification::centredLeft);
    }

private:
    Icon icon;
};

//==============================================================================
// Preset list (left column)
class PresetList : public Component
{
public:
    std::function<void (int)> onSelect;      // row index
    int selected = 0;

    PresetList() { rows = { { "Hip-Hop Classics", Icon::Crown }, { "Boom Bap", Icon::Boombox }, { "Trap / Modern", Icon::Cap },
                            { "Commercial", Icon::Diamond }, { "DJ Cuts", Icon::Vinyl }, { "User Presets", Icon::Folder } }; }

    void paint (Graphics& g) override
    {
        const float rh = (float) getHeight() / (float) rows.size();
        for (int i = 0; i < (int) rows.size(); ++i)
        {
            auto r = Rectangle<float> (0, (float) i * rh, (float) getWidth(), rh).reduced (2.0f, 2.0f);
            const bool sel = i == selected, hov = i == hover;
            if (sel)
            {
                Path p;
                p.startNewSubPath (r.getX(), r.getY()); p.lineTo (r.getRight() - 14.0f, r.getY()); p.lineTo (r.getRight(), r.getCentreY());
                p.lineTo (r.getRight() - 14.0f, r.getBottom()); p.lineTo (r.getX(), r.getBottom()); p.closeSubPath();
                g.setColour (Col::violet.withAlpha (0.3f)); g.strokePath (p, PathStrokeType (7.0f));
                g.setGradientFill (ColourGradient (Col::violetBright, r.getX(), 0, Col::violet, r.getRight(), 0, false));
                g.fillPath (p);
            }
            else if (hov)
            {
                g.setColour (Col::violet.withAlpha (0.18f));
                g.fillRoundedRectangle (r, 6.0f);
            }
            drawIcon (g, rows[(size_t) i].icon, { r.getX() + 10.0f, r.getCentreY() - 14.0f, 28.0f, 28.0f },
                      sel ? Colours::white : Col::lavender);
            g.setColour (sel ? Colours::white : Col::text);
            g.setFont (Fonts::labelBold (25.0f));
            g.drawText (rows[(size_t) i].name, r.withTrimmedLeft (54.0f), Justification::centredLeft);
            if (i == (int) rows.size() - 1)
            {
                g.setColour (Col::dim);
                g.drawText (">", r.withTrimmedRight (10.0f), Justification::centredRight);
            }
        }
    }

    void mouseMove (const MouseEvent& e) override  { setHover (indexAt (e.y)); }
    void mouseExit (const MouseEvent&) override    { setHover (-1); }
    void mouseDown (const MouseEvent& e) override
    {
        const int i = indexAt (e.y);
        if (i >= 0) { selected = i; repaint(); if (onSelect) onSelect (i); }
    }

private:
    struct Row { String name; Icon icon; };
    std::vector<Row> rows;
    int hover = -1;
    int indexAt (int y) const { return jlimit (0, (int) rows.size() - 1, (int) ((float) y / ((float) getHeight() / (float) rows.size()))); }
    void setHover (int i) { if (hover != i) { hover = i; repaint(); } }
};

//==============================================================================
// Rotary knob with a title above and the value text below
class KnobCell : public Component
{
public:
    KnobCell (AudioProcessorValueTreeState& apvts, const String& paramID, const String& title)
        : name (title)
    {
        slider.setSliderStyle (Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        slider.setVelocityBasedMode (false);
        slider.setMouseDragSensitivity (180);
        addAndMakeVisible (slider);
        attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (apvts, paramID, slider);
        slider.onValueChange = [this] { repaint(); };
        if (auto* p = apvts.getParameter (paramID)) slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b.removeFromTop (30);
        b.removeFromBottom (30);
        slider.setBounds (b.withSizeKeepingCentre (jmin (b.getWidth(), b.getHeight()), jmin (b.getWidth(), b.getHeight())));
    }

    void paint (Graphics& g) override
    {
        g.setColour (Col::white);
        g.setFont (Fonts::labelBold (26.0f));
        g.drawText (name, getLocalBounds().removeFromTop (30), Justification::centred);
        g.setColour (Col::text);
        g.setFont (Fonts::labelBold (25.0f));
        g.drawText (slider.getTextFromValue (slider.getValue()), getLocalBounds().removeFromBottom (30), Justification::centred);
    }

    Slider slider;
private:
    String name;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
// Titled drop-down (Rhythm, Cuts, Variation, Pitch)
class ComboCell : public Component
{
public:
    ComboCell (AudioProcessorValueTreeState& apvts, const String& paramID, const String& title, Icon i)
        : name (title), icon (i)
    {
        if (auto* p = dynamic_cast<AudioParameterChoice*> (apvts.getParameter (paramID)))
            combo.addItemList (p->choices, 1);
        combo.setJustificationType (Justification::centredLeft);
        addAndMakeVisible (combo);
        attachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, paramID, combo);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b.removeFromTop (30);
        combo.setBounds (b.reduced (6, 2));
    }

    void paint (Graphics& g) override
    {
        drawIcon (g, icon, { 14.0f, 4.0f, 22.0f, 22.0f }, Col::violetBright, 1.6f);
        g.setColour (Col::text);
        g.setFont (Fonts::labelBold (24.0f));
        g.drawText (name, 44, 0, getWidth() - 50, 30, Justification::centredLeft);
    }

private:
    String name; Icon icon;
    ComboBox combo;
    std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

//==============================================================================
class SwitchButton : public ToggleButton
{
public:
    void paintButton (Graphics& g, bool, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (3.0f);
        const bool on = getToggleState();
        g.setColour (on ? Col::violet.withAlpha (0.35f) : Colours::transparentBlack);
        g.fillRoundedRectangle (r.expanded (3.0f), r.getHeight());
        g.setGradientFill (ColourGradient (on ? Col::violetBright : Colour (0xff2a2140), 0, r.getY(), on ? Col::violetDark : Colour (0xff0d0916), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (Colours::white.withAlpha (0.6f));
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.4f);
        const float d = r.getHeight() - 6.0f;
        const float x = on ? r.getRight() - d - 3.0f : r.getX() + 3.0f;
        g.setColour (Colours::white);
        g.fillEllipse (x, r.getY() + 3.0f, d, d);
    }
};

class CheckRow : public ToggleButton
{
public:
    explicit CheckRow (const String& t) { setButtonText (t); }
    void paintButton (Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat();
        auto box = Rectangle<float> (4.0f, r.getCentreY() - 17.0f, 34.0f, 34.0f);
        const bool on = getToggleState();
        g.setGradientFill (ColourGradient (on ? Col::violetBright : Colour (0xff1a1230), 0, box.getY(), on ? Col::violetDark : Colour (0xff0b0714), 0, box.getBottom(), false));
        g.fillRoundedRectangle (box, 7.0f);
        g.setColour (over ? Colours::white : Col::lavender.withAlpha (0.8f));
        g.drawRoundedRectangle (box, 7.0f, 1.6f);
        if (on)
        {
            Path c;
            c.startNewSubPath (box.getX() + 8.0f, box.getCentreY());
            c.lineTo (box.getX() + 15.0f, box.getBottom() - 9.0f);
            c.lineTo (box.getRight() - 7.0f, box.getY() + 8.0f);
            g.setColour (Colours::white);
            g.strokePath (c, PathStrokeType (3.4f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (Col::text);
        g.setFont (Fonts::labelBold (26.0f));
        g.drawText (getButtonText(), r.withTrimmedLeft (52.0f), Justification::centredLeft);
    }
};

class HostToggle : public ToggleButton
{
public:
    void paintButton (Graphics& g, bool, bool) override
    {
        auto r = getLocalBounds().toFloat();
        const bool on = getToggleState();
        g.setColour (on ? Col::violetBright : Col::violetDark);
        g.fillEllipse (4.0f, r.getCentreY() - 6.0f, 12.0f, 12.0f);
        if (on) { g.setColour (Col::violet.withAlpha (0.4f)); g.fillEllipse (0.0f, r.getCentreY() - 10.0f, 20.0f, 20.0f); }
        g.setColour (on ? Col::white : Col::dim);
        g.setFont (Fonts::labelBold (22.0f));
        g.drawText ("HOST SYNC", r.withTrimmedLeft (26.0f), Justification::centredLeft);
    }
};

//==============================================================================
// Stop-type tiles + restart
class StopTiles : public Component
{
public:
    explicit StopTiles (AudioProcessorValueTreeState& apvts)
    {
        typeAtt = std::make_unique<ParameterAttachment> (*apvts.getParameter ("stopType"),
                    [this] (float v) { type = roundToInt (v); repaint(); });
        restartAtt = std::make_unique<ParameterAttachment> (*apvts.getParameter ("restart"),
                    [this] (float v) { restart = v > 0.5f; repaint(); });
        typeAtt->sendInitialUpdate();
        restartAtt->sendInitialUpdate();
    }

    void paint (Graphics& g) override
    {
        for (int i = 0; i < 7; ++i)
        {
            auto r = tileBounds (i);
            const bool sel = (i < 6) ? (i == type) : restart;
            const bool hov = (i == hover);
            auto path = chamferedRect (r, 9.0f);
            if (sel)
            {
                g.setColour (Col::violet.withAlpha (0.35f));
                g.strokePath (path, PathStrokeType (8.0f));
            }
            g.setGradientFill (ColourGradient (sel ? Colour (0xff2a1850) : Colour (0xff18102a), 0, r.getY(), Colour (0xff09060f), 0, r.getBottom(), false));
            g.fillPath (path);
            g.setColour (sel ? Colours::white.withAlpha (0.9f) : (hov ? Col::violetBright : Col::violet.withAlpha (0.55f)));
            g.strokePath (path, PathStrokeType (sel ? 2.0f : 1.3f));

            static const Icon icons[7] = { Icon::Vinyl, Icon::Cassette, Icon::Reel, Icon::DownArrow, Icon::Stop, Icon::Rewind, Icon::Restart };
            static const char* names[7] = { "Vinyl Stop", "Tape Stop", "Slow Motion", "Deep Stop", "Hard Stop", "Reverse Stop", "Tape Restart" };
            drawIcon (g, icons[i], r.withTrimmedBottom (34.0f).reduced (22.0f, 10.0f), sel ? Colours::white : Col::lavender, 2.0f);
            g.setColour (sel ? Colours::white : Col::text);
            g.setFont (Fonts::labelBold (21.0f));
            g.drawText (names[i], r.removeFromBottom (32.0f), Justification::centred);
        }
    }

    void mouseMove (const MouseEvent& e) override { setHover (indexAt (e.position)); }
    void mouseExit (const MouseEvent&) override   { setHover (-1); }
    void mouseDown (const MouseEvent& e) override
    {
        const int i = indexAt (e.position);
        if (i < 0) return;
        if (i < 6) typeAtt->setValueAsCompleteGesture ((float) i);
        else       restartAtt->setValueAsCompleteGesture (restart ? 0.0f : 1.0f);
    }

private:
    Rectangle<float> tileBounds (int i) const
    {
        const float gap = 8.0f;
        const float w = ((float) getWidth() - gap * 6.0f) / 7.0f;
        return { (float) i * (w + gap), 0.0f, w, (float) getHeight() };
    }
    int indexAt (Point<float> p) const { for (int i = 0; i < 7; ++i) if (tileBounds (i).contains (p)) return i; return -1; }
    void setHover (int i) { if (hover != i) { hover = i; repaint(); } }

    int type = 0, hover = -1; bool restart = false;
    std::unique_ptr<ParameterAttachment> typeAtt, restartAtt;
};

//==============================================================================
// Braking curve display (rate vs time) with a draggable handle
class CurveView : public Component, private Timer
{
public:
    explicit CurveView (AudioProcessorValueTreeState& a) : apvts (a)
    {
        curveAtt = std::make_unique<ParameterAttachment> (*apvts.getParameter ("curve"), [this] (float v) { curve = v; repaint(); });
        curveAtt->sendInitialUpdate();
        startTimerHz (15);
    }

    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (4.0f);
        g.setColour (Colour (0xff07040c));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (Col::violetDark.withAlpha (0.8f));
        for (int i = 1; i < 4; ++i)
        {
            g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / 4.0f), r.getY(), r.getBottom());
            g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / 4.0f), r.getX(), r.getRight());
        }
        g.setColour (Col::violet.withAlpha (0.7f));
        g.drawRoundedRectangle (r, 5.0f, 1.2f);

        Path p;
        for (int i = 0; i <= 80; ++i)
        {
            const double x = (double) i / 80.0;
            const auto pt = toPoint (x, rateAt (x), r);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (Col::violet.withAlpha (0.35f));
        g.strokePath (p, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Col::lavender);
        g.strokePath (p, PathStrokeType (2.4f, PathStrokeType::curved, PathStrokeType::rounded));

        for (auto x : { 0.0, 1.0 })
        {
            const auto pt = toPoint (x, rateAt (x), r);
            g.setColour (Col::lavender); g.fillEllipse (pt.x - 4.0f, pt.y - 4.0f, 8.0f, 8.0f);
        }
        const auto h = handlePoint();
        g.setColour (Col::violetBright.withAlpha (0.45f)); g.fillEllipse (h.x - 11.0f, h.y - 11.0f, 22.0f, 22.0f);
        g.setColour (Colours::white); g.fillEllipse (h.x - 6.0f, h.y - 6.0f, 12.0f, 12.0f);
        g.setColour (Col::violet); g.drawEllipse (h.x - 6.0f, h.y - 6.0f, 12.0f, 12.0f, 2.0f);
    }

    void mouseDown (const MouseEvent&) override { curveAtt->beginGesture(); }
    void mouseUp (const MouseEvent&) override   { curveAtt->endGesture(); }
    void mouseDrag (const MouseEvent& e) override
    {
        auto r = getLocalBounds().toFloat().reduced (4.0f);
        const double yn = 1.0 - jlimit (0.0, 1.0, (double) ((e.position.y - r.getY()) / r.getHeight()));
        const double rate = yn * (yMax() - yMin()) + yMin();
        const int type = roundToInt (apvts.getRawParameterValue ("stopType")->load());
        const double drop = jmin (0.0f, apvts.getRawParameterValue ("pitchDrop")->load()) + (type == 3 ? -12.0 : 0.0);
        static const double mul[] = { 1.0, 1.5, 0.8, 2.2, 1.0, 1.0 };
        double c;
        if (type == 5)
        {
            const double s = jlimit (1e-4, 0.9999, (1.0 - rate) / 2.6);
            c = std::log (s) / std::log (0.5) - 0.9;
        }
        else
        {
            if (drop > -0.5) return;
            const double s = jlimit (1e-4, 0.9999, std::log2 (jmax (1e-4, rate)) * 12.0 / drop);
            const double shape = std::log (s) / std::log (0.5);
            c = (shape / mul[jlimit (0, 5, type)] - 0.5) / 2.5;
        }
        curveAtt->setValueAsPartOfGesture ((float) jlimit (0.0, 1.0, c));
    }

private:
    void timerCallback() override { repaint(); }

    bs::StopType type() const { return (bs::StopType) roundToInt (apvts.getRawParameterValue ("stopType")->load()); }
    double rateAt (double x) const { return bs::stopRateAt (type(), x, apvts.getRawParameterValue ("pitchDrop")->load(), curve); }
    double yMin() const { return type() == bs::StopType::Reverse ? -1.8 : -0.05; }
    double yMax() const { return 1.08; }
    Point<float> toPoint (double x, double rate, Rectangle<float> r) const
    {
        const double yn = (rate - yMin()) / (yMax() - yMin());
        return { r.getX() + (float) x * r.getWidth(), r.getBottom() - (float) jlimit (0.0, 1.0, yn) * r.getHeight() };
    }
    Point<float> handlePoint() const { return toPoint (0.5, rateAt (0.5), getLocalBounds().toFloat().reduced (4.0f)); }

    AudioProcessorValueTreeState& apvts;
    float curve = 0.5f;
    std::unique_ptr<ParameterAttachment> curveAtt;
};

//==============================================================================
class LevelMeter : public Component
{
public:
    explicit LevelMeter (BlessedScratchProcessor& p) : proc (p) {}

    void tick()
    {
        shownL = jmax (proc.levelL.exchange (0.0f), shownL * 0.86f);
        shownR = jmax (proc.levelR.exchange (0.0f), shownR * 0.86f);
        repaint();
    }

    void paint (Graphics& g) override
    {
        const int segs = 16;
        const float w = ((float) getWidth() - 4.0f) * 0.5f;
        const float sh = (float) getHeight() / (float) segs;
        for (int ch = 0; ch < 2; ++ch)
        {
            const float dB = Decibels::gainToDecibels (ch == 0 ? shownL : shownR, -60.0f);
            const int lit = jlimit (0, segs, (int) std::ceil ((dB + 48.0f) / 48.0f * (float) segs));
            for (int s = 0; s < segs; ++s)
            {
                const bool on = s < lit;
                const float t = (float) s / (float) (segs - 1);
                auto col = Col::violetDark.interpolatedWith (Col::violetBright, jmin (1.0f, t * 1.4f)).interpolatedWith (Colours::white, t > 0.85f ? 0.8f : 0.0f);
                g.setColour (on ? col : Colour (0xff160f24));
                g.fillRoundedRectangle ((float) ch * (w + 4.0f), (float) getHeight() - (float) (s + 1) * sh + 1.0f, w, sh - 2.0f, 1.5f);
            }
        }
    }

private:
    BlessedScratchProcessor& proc;
    float shownL = 0.0f, shownR = 0.0f;
};

//==============================================================================
class WaveformView : public Component, public FileDragAndDropTarget, private Timer
{
public:
    explicit WaveformView (BlessedScratchProcessor& p) : proc (p) { startTimerHz (30); }

    void setShowResult (bool r) { showResult = r; dirty = true; repaint(); }
    bool getShowResult() const  { return showResult; }
    void showMessage (const String& m, int ms = 3500) { message = m; messageUntil = Time::getMillisecondCounter() + (uint32) ms; repaint(); }

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto path = chamferedRect (b.reduced (2.0f), 16.0f);
        g.setGradientFill (ColourGradient (Colour (0xff0e0818), 0, 0, Colour (0xff05030a), 0, b.getBottom(), false));
        g.fillPath (path);
        g.setColour (Col::violet.withAlpha (0.2f)); g.strokePath (path, PathStrokeType (6.0f));
        g.setColour (Col::violet); g.strokePath (path, PathStrokeType (1.6f));

        auto w = waveArea();
        const bool hasAudio = ! peaks.empty() && (showResult ? proc.getDisplayResult() != nullptr : proc.getCaptureLength() > 0);

        if (hasAudio)
        {
            const float selA = proc.selStart.load(), selB = proc.selEnd.load();
            const float bw = 4.0f;
            const int n = (int) peaks.size();
            for (int i = 0; i < n; ++i)
            {
                const float x = w.getX() + (float) i * bw;
                const float xn = (float) i / (float) n;
                const float h = jmax (2.5f, peaks[(size_t) i] * w.getHeight() * 0.94f);
                const bool inside = showResult || (xn >= selA && xn <= selB);
                g.setColour (inside ? Col::lavender.interpolatedWith (Col::violetBright, 0.25f + 0.5f * (1.0f - peaks[(size_t) i]))
                                    : Col::violet.withAlpha (0.65f));
                g.fillRoundedRectangle (x, w.getCentreY() - h * 0.5f, 2.6f, h, 1.3f);
            }

            if (! showResult)
            {
                auto sel = Rectangle<float> (w.getX() + selA * w.getWidth(), w.getY(), (selB - selA) * w.getWidth(), w.getHeight());
                g.setColour (Col::violet.withAlpha (0.10f)); g.fillRect (sel);
                g.setColour (Colours::white.withAlpha (0.95f)); g.drawRect (sel, 1.6f);
                for (auto pt : { sel.getTopLeft(), sel.getTopRight(), sel.getBottomLeft(), sel.getBottomRight() })
                {
                    g.setColour (Col::violetBright.withAlpha (0.5f)); g.fillEllipse (pt.x - 9.0f, pt.y - 9.0f, 18.0f, 18.0f);
                    g.setColour (Colours::white);                    g.fillEllipse (pt.x - 5.5f, pt.y - 5.5f, 11.0f, 11.0f);
                    g.setColour (Col::violet);                       g.drawEllipse (pt.x - 5.5f, pt.y - 5.5f, 11.0f, 11.0f, 1.8f);
                }
            }
            else if (proc.isPreviewing())
            {
                const float x = w.getX() + proc.getPreviewPhase() * w.getWidth();
                g.setColour (Colours::white); g.fillRect (x - 1.0f, w.getY(), 2.0f, w.getHeight());
            }
        }

        g.setFont (Fonts::labelBold (22.0f));
        if (proc.isRecording())
        {
            g.setColour (Col::red); g.fillEllipse (w.getX() + 6.0f, w.getY() + 6.0f, 14.0f, 14.0f);
            const double secs = (double) proc.getCaptureLength() / jmax (1.0, proc.getSampleRate());
            g.setColour (Colours::white);
            g.drawText ("REC  " + String (secs, 1) + " s", (int) w.getX() + 26, (int) w.getY() + 2, 160, 24, Justification::centredLeft);
        }
        else if (hasAudio)
        {
            g.setColour (Col::dim);
            g.drawText (showResult ? "RESULT" : "SOURCE  -  drag the box to choose the region", (int) w.getX() + 6, (int) w.getY() + 2, 420, 24, Justification::centredLeft);
        }

        const bool msgOn = Time::getMillisecondCounter() < messageUntil;
        if (! hasAudio || msgOn)
        {
            g.setColour (msgOn ? Col::white : Col::dim);
            g.setFont (Fonts::labelBold (28.0f));
            g.drawText (msgOn ? message : (showResult ? "Press GENERATE to create a pattern" : "Press REC and play audio through the plugin, or LOAD / drop an audio file"),
                        b.toNearestInt(), Justification::centred);
        }
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (showResult || proc.getCaptureLength() <= 0) return;
        const float a = proc.selStart.load(), b = proc.selEnd.load(), x = toNorm (e.position.x);
        const float tol = 12.0f / jmax (1.0f, waveArea().getWidth());
        if (std::abs (x - a) < tol)      mode = 1;
        else if (std::abs (x - b) < tol) mode = 2;
        else if (x > a && x < b)         { mode = 3; grab = x - a; }
        else                             { mode = 4; anchor = x; proc.selStart = x; proc.selEnd = jmin (1.0f, x + 0.02f); }
        repaint();
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (mode == 0) return;
        const float x = toNorm (e.position.x), minW = 0.01f;
        float a = proc.selStart.load(), b = proc.selEnd.load();
        if (mode == 1)      a = jmin (x, b - minW);
        else if (mode == 2) b = jmax (x, a + minW);
        else if (mode == 3) { const float len = b - a; a = jlimit (0.0f, 1.0f - len, x - grab); b = a + len; }
        else if (mode == 4) { a = jmin (anchor, x); b = jmax (anchor, x + minW); }
        proc.selStart = jlimit (0.0f, 1.0f, a);
        proc.selEnd = jlimit (0.0f, 1.0f, b);
        repaint();
    }

    void mouseUp (const MouseEvent&) override { mode = 0; }

    void mouseMove (const MouseEvent& e) override
    {
        if (showResult || proc.getCaptureLength() <= 0) { setMouseCursor (MouseCursor::NormalCursor); return; }
        const float x = toNorm (e.position.x), tol = 12.0f / jmax (1.0f, waveArea().getWidth());
        const bool edge = std::abs (x - proc.selStart.load()) < tol || std::abs (x - proc.selEnd.load()) < tol;
        setMouseCursor (edge ? MouseCursor::LeftRightResizeCursor : MouseCursor::NormalCursor);
    }

    void resized() override { dirty = true; }

    // drag & drop of audio files
    bool isInterestedInFileDrag (const StringArray& files) override
    {
        for (auto& f : files)
            if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3")) return true;
        return false;
    }
    void filesDropped (const StringArray& files, int, int) override
    {
        if (! files.isEmpty())
            showMessage (proc.loadAudioFile (File (files[0])) ? "Audio loaded" : "Could not read that file");
    }

private:
    Rectangle<float> waveArea() const { return getLocalBounds().toFloat().reduced (20.0f, 22.0f); }
    float toNorm (float x) const { auto w = waveArea(); return jlimit (0.0f, 1.0f, (x - w.getX()) / jmax (1.0f, w.getWidth())); }

    void timerCallback() override
    {
        const auto cv = proc.captureVersion.load();
        const auto rv = proc.resultVersion.load();
        if (cv != lastCapture || rv != lastResult || dirty)
        {
            lastCapture = cv; lastResult = rv; dirty = false;
            refreshPeaks();
        }
        repaint();
    }

    void refreshPeaks()
    {
        const int bars = jmax (1, (int) (waveArea().getWidth() / 4.0f));
        if (! showResult) { proc.getPeaks (peaks, bars); }
        else
        {
            peaks.assign ((size_t) bars, 0.0f);
            if (auto r = proc.getDisplayResult())
            {
                const size_t n = r->audio.size();
                if (n > 0)
                    for (int i = 0; i < bars; ++i)
                    {
                        const size_t s0 = (size_t) ((double) i / bars * (double) n), s1 = jmax (s0 + 1, (size_t) ((double) (i + 1) / bars * (double) n));
                        const size_t stride = jmax<size_t> (1, (s1 - s0) / 48);
                        float m = 0.0f;
                        for (size_t s = s0; s < jmin (s1, n); s += stride) m = jmax (m, std::abs (r->audio.ch[0][s]), std::abs (r->audio.ch[1][s]));
                        peaks[(size_t) i] = m;
                    }
            }
        }
        float mx = 0.0f; for (auto v : peaks) mx = jmax (mx, v);
        if (mx > 1.0e-4f) for (auto& v : peaks) v = jmax (0.04f, v / mx);
        else              for (auto& v : peaks) v = 0.0f;
    }

    BlessedScratchProcessor& proc;
    std::vector<float> peaks;
    bool showResult = false, dirty = true;
    uint32_t lastCapture = 0xffffffff, lastResult = 0xffffffff;
    int mode = 0; float grab = 0.0f, anchor = 0.0f;
    String message; uint32 messageUntil = 0;
};
} // namespace bsui

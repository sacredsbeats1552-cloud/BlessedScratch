#pragma once
//==============================================================================
//  BLESSED SCRATCH - shared look & feel, colours, fonts and icon drawing
//==============================================================================
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "BinaryData.h"

namespace bsui
{
using namespace juce;

namespace Col
{
    const Colour bg0          { 0xff050308 };
    const Colour panelTop     { 0xff150c24 };
    const Colour panelBot     { 0xff08050e };
    const Colour violet       { 0xff8b3dff };
    const Colour violetBright { 0xffb26bff };
    const Colour violetDark   { 0xff3b1578 };
    const Colour lavender     { 0xffd8c6ff };
    const Colour white        { 0xfff5efff };
    const Colour text         { 0xffeadfff };
    const Colour dim          { 0xff8d79b3 };
    const Colour red          { 0xffff4a6b };
}

struct Fonts
{
    static Typeface::Ptr anton()
    {
        static Typeface::Ptr t = Typeface::createSystemTypefaceFor (BinaryData::AntonRegular_ttf, BinaryData::AntonRegular_ttfSize);
        return t;
    }
    static Typeface::Ptr barlow()
    {
        static Typeface::Ptr t = Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedMedium_ttf, BinaryData::BarlowCondensedMedium_ttfSize);
        return t;
    }
    static Typeface::Ptr barlowBold()
    {
        static Typeface::Ptr t = Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedSemiBold_ttf, BinaryData::BarlowCondensedSemiBold_ttfSize);
        return t;
    }
    static Font heading (float h)   { return Font (FontOptions (anton()).withHeight (h)); }
    static Font label (float h)     { return Font (FontOptions (barlow()).withHeight (h)); }
    static Font labelBold (float h) { return Font (FontOptions (barlowBold()).withHeight (h)); }
};

inline float textWidth (const Font& f, const String& s)
{
    GlyphArrangement ga;
    ga.addLineOfText (f, s, 0.0f, 0.0f);
    return ga.getBoundingBox (0, -1, true).getWidth();
}

//------------------------------------------------------------------------------
inline Path chamferedRect (Rectangle<float> r, float cut)
{
    Path p;
    p.startNewSubPath (r.getX() + cut, r.getY());
    p.lineTo (r.getRight() - cut, r.getY());
    p.lineTo (r.getRight(), r.getY() + cut);
    p.lineTo (r.getRight(), r.getBottom() - cut);
    p.lineTo (r.getRight() - cut, r.getBottom());
    p.lineTo (r.getX() + cut, r.getBottom());
    p.lineTo (r.getX(), r.getBottom() - cut);
    p.lineTo (r.getX(), r.getY() + cut);
    p.closeSubPath();
    return p.createPathWithRoundedCorners (3.0f);
}

inline void drawPanel (Graphics& g, Rectangle<float> r, bool active = true, float cut = 14.0f)
{
    auto path = chamferedRect (r, cut);
    g.setGradientFill (ColourGradient (Col::panelTop, r.getX(), r.getY(), Col::panelBot, r.getX(), r.getBottom(), false));
    g.fillPath (path);
    if (active)
    {
        g.setColour (Col::violet.withAlpha (0.16f));
        g.strokePath (path, PathStrokeType (6.0f));
    }
    g.setColour (active ? Col::violet.withAlpha (0.95f) : Col::violetDark.withAlpha (0.9f));
    g.strokePath (path, PathStrokeType (1.6f));
    g.setColour (Colours::white.withAlpha (0.05f));
    g.strokePath (chamferedRect (r.reduced (4.0f), cut - 2.0f), PathStrokeType (1.0f));
}

//------------------------------------------------------------------------------
enum class Icon { Crown, Boombox, Cap, Diamond, Vinyl, Folder, Cassette, Reel, DownArrow, Stop, Rewind,
                  Restart, Play, Refresh, Headphones, Rec, Load, Export, Clear, View, Bars, Star,
                  Note, Cuts, Dice, Pitch };

inline void drawIcon (Graphics& g, Icon icon, Rectangle<float> r, Colour c, float stroke = 1.8f)
{
    const auto cx = r.getCentreX(), cy = r.getCentreY();
    const float s = jmin (r.getWidth(), r.getHeight());
    const float h = s * 0.5f;
    g.setColour (c);
    PathStrokeType st (stroke, PathStrokeType::curved, PathStrokeType::rounded);

    switch (icon)
    {
        case Icon::Crown:
        {
            Path p;
            p.startNewSubPath (cx - h, cy + h * 0.6f);
            p.lineTo (cx - h, cy - h * 0.5f);
            p.lineTo (cx - h * 0.45f, cy + h * 0.05f);
            p.lineTo (cx, cy - h * 0.7f);
            p.lineTo (cx + h * 0.45f, cy + h * 0.05f);
            p.lineTo (cx + h, cy - h * 0.5f);
            p.lineTo (cx + h, cy + h * 0.6f);
            p.closeSubPath();
            g.fillPath (p.createPathWithRoundedCorners (2.0f));
            break;
        }
        case Icon::Boombox:
            g.drawRoundedRectangle (cx - h, cy - h * 0.55f, s, h * 1.2f, 3.0f, stroke);
            g.drawEllipse (cx - h * 0.62f, cy - h * 0.2f, h * 0.55f, h * 0.55f, stroke);
            g.drawEllipse (cx + h * 0.07f, cy - h * 0.2f, h * 0.55f, h * 0.55f, stroke);
            g.drawLine (cx - h * 0.6f, cy - h * 0.55f, cx - h * 0.6f, cy - h * 0.85f, stroke);
            break;
        case Icon::Cap:
        {
            Path p;
            p.addCentredArc (cx, cy + h * 0.15f, h * 0.8f, h * 0.7f, 0.0f, -MathConstants<float>::halfPi * 1.0f, MathConstants<float>::halfPi, true);
            p.closeSubPath();
            g.fillPath (p);
            g.fillRoundedRectangle (cx - h * 0.2f, cy + h * 0.1f, h * 1.2f, h * 0.28f, 2.0f);
            break;
        }
        case Icon::Diamond:
        {
            Path p;
            p.startNewSubPath (cx - h * 0.6f, cy - h * 0.4f);
            p.lineTo (cx + h * 0.6f, cy - h * 0.4f);
            p.lineTo (cx + h, cy);
            p.lineTo (cx, cy + h * 0.85f);
            p.lineTo (cx - h, cy);
            p.closeSubPath();
            g.strokePath (p, st);
            g.drawLine (cx - h, cy, cx + h, cy, stroke * 0.7f);
            break;
        }
        case Icon::Vinyl:
            g.drawEllipse (cx - h, cy - h, s, s, stroke);
            g.drawEllipse (cx - h * 0.55f, cy - h * 0.55f, h * 1.1f, h * 1.1f, stroke * 0.7f);
            g.fillEllipse (cx - h * 0.16f, cy - h * 0.16f, h * 0.32f, h * 0.32f);
            break;
        case Icon::Folder:
        {
            Path p;
            p.addRoundedRectangle (cx - h, cy - h * 0.55f, s, h * 1.35f, 3.0f);
            g.fillPath (p);
            g.fillRoundedRectangle (cx - h, cy - h * 0.8f, h * 0.9f, h * 0.35f, 2.0f);
            break;
        }
        case Icon::Cassette:
            g.drawRoundedRectangle (cx - h, cy - h * 0.62f, s, h * 1.24f, 4.0f, stroke);
            g.drawEllipse (cx - h * 0.62f, cy - h * 0.25f, h * 0.45f, h * 0.45f, stroke);
            g.drawEllipse (cx + h * 0.17f, cy - h * 0.25f, h * 0.45f, h * 0.45f, stroke);
            g.drawLine (cx - h * 0.4f, cy + h * 0.42f, cx + h * 0.4f, cy + h * 0.42f, stroke);
            break;
        case Icon::Reel:
            for (int k = -1; k <= 1; k += 2)
            {
                const float rx = cx + (float) k * h * 0.5f, rr = h * 0.5f;
                g.drawEllipse (rx - rr, cy - h * 0.45f - rr * 0.0f, rr * 2.0f, rr * 2.0f, stroke);
                g.fillEllipse (rx - 2.5f, cy - h * 0.45f + rr - 2.5f, 5.0f, 5.0f);
            }
            g.drawLine (cx - h * 0.5f, cy + h * 0.6f, cx + h * 0.5f, cy + h * 0.6f, stroke);
            break;
        case Icon::DownArrow:
        {
            Path p;
            p.startNewSubPath (cx - h * 0.7f, cy - h * 0.7f); p.lineTo (cx, cy - h * 0.05f); p.lineTo (cx + h * 0.7f, cy - h * 0.7f);
            p.startNewSubPath (cx - h * 0.7f, cy - h * 0.05f); p.lineTo (cx, cy + h * 0.6f); p.lineTo (cx + h * 0.7f, cy - h * 0.05f);
            g.strokePath (p, PathStrokeType (stroke * 1.4f, PathStrokeType::curved, PathStrokeType::rounded));
            g.fillRoundedRectangle (cx - h * 0.7f, cy + h * 0.78f, h * 1.4f, stroke * 1.4f, 1.5f);
            break;
        }
        case Icon::Stop:
            g.drawRoundedRectangle (cx - h * 0.9f, cy - h * 0.9f, h * 1.8f, h * 1.8f, 6.0f, stroke);
            g.fillRoundedRectangle (cx - h * 0.42f, cy - h * 0.42f, h * 0.84f, h * 0.84f, 2.0f);
            break;
        case Icon::Rewind:
        {
            for (int k = 0; k < 2; ++k)
            {
                const float ox = cx + ((float) k - 0.5f) * h * 1.05f;
                Path p;
                p.addTriangle (ox + h * 0.5f, cy - h * 0.6f, ox + h * 0.5f, cy + h * 0.6f, ox - h * 0.5f, cy);
                g.fillPath (p);
            }
            break;
        }
        case Icon::Restart:
        {
            Path p;
            p.addCentredArc (cx, cy, h * 0.8f, h * 0.8f, 0.0f, 0.6f, MathConstants<float>::twoPi - 0.4f, true);
            g.strokePath (p, st);
            Path t; t.addTriangle (cx + h * 0.55f, cy - h * 0.95f, cx + h * 0.95f, cy - h * 0.35f, cx + h * 0.2f, cy - h * 0.4f);
            g.fillPath (t);
            break;
        }
        case Icon::Play:
        {
            Path p; p.addTriangle (cx - h * 0.6f, cy - h * 0.8f, cx - h * 0.6f, cy + h * 0.8f, cx + h * 0.85f, cy);
            g.fillPath (p.createPathWithRoundedCorners (2.0f));
            break;
        }
        case Icon::Refresh:
        {
            Path p;
            p.addCentredArc (cx, cy, h * 0.7f, h * 0.7f, 0.0f, 0.3f, 4.2f, true);
            g.strokePath (p, PathStrokeType (stroke * 1.4f, PathStrokeType::curved, PathStrokeType::rounded));
            Path p2;
            p2.addCentredArc (cx, cy, h * 0.7f, h * 0.7f, 0.0f, 3.45f, 7.3f, true);
            g.strokePath (p2, PathStrokeType (stroke * 1.4f, PathStrokeType::curved, PathStrokeType::rounded));
            Path t1; t1.addTriangle (cx + h * 0.95f, cy - h * 0.1f, cx + h * 0.45f, cy - h * 0.1f, cx + h * 0.7f, cy + h * 0.4f);
            Path t2; t2.addTriangle (cx - h * 0.95f, cy + h * 0.1f, cx - h * 0.45f, cy + h * 0.1f, cx - h * 0.7f, cy - h * 0.4f);
            g.fillPath (t1); g.fillPath (t2);
            break;
        }
        case Icon::Headphones:
        {
            Path p;
            p.addCentredArc (cx, cy + h * 0.1f, h * 0.8f, h * 0.85f, 0.0f, -MathConstants<float>::halfPi - 1.45f, -MathConstants<float>::halfPi + 1.45f, true);
            g.strokePath (p, PathStrokeType (stroke * 1.5f, PathStrokeType::curved, PathStrokeType::rounded));
            g.fillRoundedRectangle (cx - h * 0.95f, cy + h * 0.05f, h * 0.42f, h * 0.8f, 3.0f);
            g.fillRoundedRectangle (cx + h * 0.53f, cy + h * 0.05f, h * 0.42f, h * 0.8f, 3.0f);
            break;
        }
        case Icon::Rec:
            g.fillEllipse (cx - h * 0.55f, cy - h * 0.55f, h * 1.1f, h * 1.1f);
            break;
        case Icon::Load:
        {
            Path p; p.startNewSubPath (cx, cy + h * 0.5f); p.lineTo (cx, cy - h * 0.7f);
            p.startNewSubPath (cx - h * 0.5f, cy - h * 0.2f); p.lineTo (cx, cy - h * 0.7f); p.lineTo (cx + h * 0.5f, cy - h * 0.2f);
            g.strokePath (p, st);
            g.drawLine (cx - h * 0.8f, cy + h * 0.8f, cx + h * 0.8f, cy + h * 0.8f, stroke);
            break;
        }
        case Icon::Export:
        {
            Path p; p.startNewSubPath (cx, cy - h * 0.7f); p.lineTo (cx, cy + h * 0.5f);
            p.startNewSubPath (cx - h * 0.5f, cy + h * 0.05f); p.lineTo (cx, cy + h * 0.55f); p.lineTo (cx + h * 0.5f, cy + h * 0.05f);
            g.strokePath (p, st);
            g.drawLine (cx - h * 0.8f, cy + h * 0.85f, cx + h * 0.8f, cy + h * 0.85f, stroke);
            break;
        }
        case Icon::Clear:
            g.drawLine (cx - h * 0.6f, cy - h * 0.6f, cx + h * 0.6f, cy + h * 0.6f, stroke * 1.4f);
            g.drawLine (cx - h * 0.6f, cy + h * 0.6f, cx + h * 0.6f, cy - h * 0.6f, stroke * 1.4f);
            break;
        case Icon::View:
            g.drawEllipse (cx - h, cy - h * 0.55f, s, h * 1.1f, stroke);
            g.fillEllipse (cx - h * 0.28f, cy - h * 0.28f, h * 0.56f, h * 0.56f);
            break;
        case Icon::Bars:
            for (int k = 0; k < 4; ++k)
            {
                const float bh = h * (0.7f + 0.5f * (float) ((k * 5) % 4) / 3.0f);
                g.fillRoundedRectangle (cx - h + (float) k * h * 0.55f, cy + h - bh * 1.4f, h * 0.38f, bh * 1.4f, 1.5f);
            }
            break;
        case Icon::Star:
        {
            Path p; p.addStar ({ cx, cy }, 5, h * 0.42f, h * 0.95f, -MathConstants<float>::halfPi);
            g.fillPath (p.createPathWithRoundedCorners (1.5f));
            break;
        }
        case Icon::Note:
            g.fillEllipse (cx - h * 0.75f, cy + h * 0.2f, h * 0.8f, h * 0.6f);
            g.fillRect (cx - h * 0.1f, cy - h * 0.8f, stroke, h * 1.3f);
            g.fillRect (cx - h * 0.1f, cy - h * 0.8f, h * 0.8f, stroke * 1.6f);
            break;
        case Icon::Cuts:
            g.drawLine (cx - h * 0.8f, cy - h * 0.8f, cx + h * 0.8f, cy + h * 0.8f, stroke * 1.3f);
            g.drawLine (cx - h * 0.8f, cy + h * 0.8f, cx + h * 0.8f, cy - h * 0.8f, stroke * 1.3f);
            g.drawEllipse (cx - h, cy - h, h * 0.55f, h * 0.55f, stroke);
            g.drawEllipse (cx + h * 0.45f, cy - h, h * 0.55f, h * 0.55f, stroke);
            break;
        case Icon::Dice:
            g.drawRoundedRectangle (cx - h * 0.8f, cy - h * 0.8f, h * 1.6f, h * 1.6f, 4.0f, stroke);
            for (auto d : { Point<float> (-0.4f, -0.4f), Point<float> (0.4f, 0.4f), Point<float> (0.0f, 0.0f), Point<float> (-0.4f, 0.4f), Point<float> (0.4f, -0.4f) })
                g.fillEllipse (cx + d.x * h - 2.0f, cy + d.y * h - 2.0f, 4.0f, 4.0f);
            break;
        case Icon::Pitch:
            g.fillEllipse (cx - h * 0.9f, cy + h * 0.2f, h * 0.75f, h * 0.55f);
            g.fillRect (cx - h * 0.25f, cy - h * 0.75f, stroke, h * 1.3f);
            g.drawLine (cx + h * 0.3f, cy + h * 0.5f, cx + h * 0.3f, cy - h * 0.7f, stroke);
            g.drawLine (cx + h * 0.05f, cy - h * 0.4f, cx + h * 0.3f, cy - h * 0.75f, stroke);
            g.drawLine (cx + h * 0.55f, cy - h * 0.4f, cx + h * 0.3f, cy - h * 0.75f, stroke);
            break;
    }
}

//==============================================================================
class BSLookAndFeel : public LookAndFeel_V4
{
public:
    BSLookAndFeel()
    {
        setColour (PopupMenu::backgroundColourId, Colour (0xff0d0816));
        setColour (PopupMenu::textColourId, Col::text);
        setColour (PopupMenu::highlightedBackgroundColourId, Col::violet);
        setColour (PopupMenu::highlightedTextColourId, Colours::white);
        setColour (ComboBox::textColourId, Col::text);
        setColour (Slider::textBoxTextColourId, Col::white);
        setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setColour (Label::textColourId, Col::text);
        setColour (TextEditor::backgroundColourId, Colour (0xff0d0816));
        setColour (TextEditor::textColourId, Col::white);
        setColour (AlertWindow::backgroundColourId, Colour (0xff120a20));
        setColour (AlertWindow::textColourId, Col::text);
        setColour (TextButton::buttonColourId, Col::violetDark);
        setDefaultSansSerifTypeface (Fonts::barlow());
    }

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider&) override
    {
        auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float d = jmin (b.getWidth(), b.getHeight());
        const auto c = b.getCentre();
        const float R = d * 0.5f;
        const float ang = a0 + pos * (a1 - a0);

        // tick marks
        g.setColour (Col::violet.withAlpha (0.35f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = a0 + (float) i / 10.0f * (a1 - a0);
            const float s = std::sin (a), co = -std::cos (a);
            g.drawLine (c.x + s * (R - 1.0f), c.y + co * (R - 1.0f), c.x + s * (R + 2.5f), c.y + co * (R + 2.5f), 1.2f);
        }

        const float arcR = R - 6.0f;
        Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, a1, true);
        g.setColour (Col::violetDark.withAlpha (0.55f));
        g.strokePath (track, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));

        Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, ang, true);
        g.setColour (Col::violet.withAlpha (0.28f));
        g.strokePath (val, PathStrokeType (10.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Col::violetBright);
        g.strokePath (val, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));

        const float br = R * 0.68f;
        g.setGradientFill (ColourGradient (Colour (0xff4a4260), c.x - br * 0.5f, c.y - br * 0.8f,
                                           Colour (0xff0b0810), c.x + br * 0.6f, c.y + br * 0.9f, false));
        g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
        g.setColour (Colour (0xff1a1426));
        g.fillEllipse (c.x - br * 0.8f, c.y - br * 0.8f, br * 1.6f, br * 1.6f);
        g.setColour (Colours::white.withAlpha (0.10f));
        g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.0f);

        const float s = std::sin (ang), co = -std::cos (ang);
        g.setColour (Col::white);
        g.drawLine (c.x + s * br * 0.25f, c.y + co * br * 0.25f, c.x + s * br * 0.88f, c.y + co * br * 0.88f, 2.6f);
    }

    void drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box) override
    {
        auto r = Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);
        g.setGradientFill (ColourGradient (Colour (0xff1b1230), 0, 0, Colour (0xff0b0714), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (box.hasKeyboardFocus (false) ? Col::violetBright : Col::violet.withAlpha (0.75f));
        g.drawRoundedRectangle (r, 7.0f, 1.4f);

        Path chev;
        const float cx = (float) width - 18.0f, cy = (float) height * 0.5f;
        chev.startNewSubPath (cx - 5.0f, cy - 2.5f); chev.lineTo (cx, cy + 3.0f); chev.lineTo (cx + 5.0f, cy - 2.5f);
        g.setColour (Col::white);
        g.strokePath (chev, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    Font getComboBoxFont (ComboBox&) override { return Fonts::labelBold (22.0f); }
    Font getPopupMenuFont() override          { return Fonts::labelBold (20.0f); }

    void positionComboBoxText (ComboBox& box, Label& label) override
    {
        label.setBounds (12, 1, box.getWidth() - 36, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    Label* createSliderTextBox (Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        if (s.getSliderStyle() == Slider::LinearBar)
        {
            l->setFont (Fonts::heading (30.0f));
            l->setJustificationType (Justification::centred);
        }
        return l;
    }

    void drawLinearSlider (Graphics& g, int x, int y, int w, int h, float, float, float,
                           Slider::SliderStyle style, Slider&) override
    {
        if (style != Slider::LinearBar) return;
        auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (1.0f);
        g.setGradientFill (ColourGradient (Colour (0xff1b1230), 0, r.getY(), Colour (0xff0a0612), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (Col::violet);
        g.drawRoundedRectangle (r, 8.0f, 1.5f);
    }

    void drawPopupMenuBackground (Graphics& g, int w, int h) override
    {
        g.fillAll (Colour (0xff0d0816));
        g.setColour (Col::violet);
        g.drawRect (0, 0, w, h, 1);
    }
};
} // namespace bsui

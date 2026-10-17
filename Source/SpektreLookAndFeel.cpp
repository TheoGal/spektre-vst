#include "SpektreLookAndFeel.h"
#include "BinaryData.h"

namespace
{
    juce::Typeface::Ptr load (const void* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }
    struct Typefaces
    {
        juce::Typeface::Ptr barlowSemi = load (BinaryData::BarlowCondensedSemiBold_ttf, BinaryData::BarlowCondensedSemiBold_ttfSize);
        juce::Typeface::Ptr barlowBold = load (BinaryData::BarlowCondensedBold_ttf,     BinaryData::BarlowCondensedBold_ttfSize);
        juce::Typeface::Ptr monoReg    = load (BinaryData::JetBrainsMonoRegular_ttf,    BinaryData::JetBrainsMonoRegular_ttfSize);
        juce::Typeface::Ptr monoSemi   = load (BinaryData::JetBrainsMonoSemiBold_ttf,   BinaryData::JetBrainsMonoSemiBold_ttfSize);
    };
    const Typefaces& tf() { static Typefaces t; return t; }
}

juce::Font Fonts::label (float h, bool bold)
{
    return juce::Font (juce::FontOptions (bold ? tf().barlowBold : tf().barlowSemi).withHeight (h));
}

juce::Font Fonts::mono (float h, bool semibold)
{
    return juce::Font (juce::FontOptions (semibold ? tf().monoSemi : tf().monoReg).withHeight (h));
}

SpektreLookAndFeel::SpektreLookAndFeel()
{
    using namespace Palette;
    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::textColourId, fg);
    setColour (juce::ComboBox::arrowColourId, muted);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, fg);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, edge);
    setColour (juce::PopupMenu::highlightedTextColourId, trace);
    setColour (juce::TextButton::buttonColourId, panel);
    setColour (juce::TextButton::buttonOnColourId, panel);
    setColour (juce::TextButton::textColourOffId, fg);
    setColour (juce::TextButton::textColourOnId, trace);
    setColour (juce::Label::textColourId, muted);
    setColour (juce::Slider::thumbColourId, trace);
    setColour (juce::Slider::trackColourId, trace);
    setColour (juce::Slider::backgroundColourId, edge);
}

juce::Font SpektreLookAndFeel::getComboBoxFont (juce::ComboBox&)   { return Fonts::mono (13.0f); }
juce::Font SpektreLookAndFeel::getPopupMenuFont()                  { return Fonts::mono (13.5f); }
juce::Font SpektreLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return Fonts::label (15.0f).withExtraKerningFactor (0.07f); }
juce::Font SpektreLookAndFeel::getLabelFont (juce::Label& l)       { return l.getFont(); }

void SpektreLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.isMouseOver (true) ? Palette::edgeHi : Palette::edge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    // chevron
    const float cx = (float) w - 12.0f, cy = (float) h * 0.5f;
    juce::Path p;
    p.startNewSubPath (cx - 4, cy - 2); p.lineTo (cx, cy + 2); p.lineTo (cx + 4, cy - 2);
    g.setColour (Palette::muted);
    g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SpektreLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (2, 1, box.getWidth() - 22, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setColour (juce::Label::textColourId, Palette::fg);
}

void SpektreLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (Palette::panel);
    g.setColour (Palette::edge);
    g.drawRect (0, 0, w, h);
}

void SpektreLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (down ? Palette::edge : Palette::panel);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (on ? Palette::trace : over ? Palette::edgeHi : Palette::edge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    // status LED, like the web buttons
    const float d = 7.0f, x = r.getX() + 12.0f, y = r.getCentreY() - d * 0.5f;
    if (on)
    {
        g.setColour (Palette::trace.withAlpha (0.35f));
        g.fillEllipse (x - 3, y - 3, d + 6, d + 6);
    }
    g.setColour (on ? Palette::trace : juce::Colour (0xff2b3444));
    g.fillEllipse (x, y, d, d);
}

void SpektreLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? Palette::trace : Palette::fg);
    g.drawText (b.getButtonText().toUpperCase(), b.getLocalBounds().withTrimmedLeft (30).withTrimmedRight (8),
                juce::Justification::centredLeft, false);
}

namespace
{
    class TitleBarButton final : public juce::Button
    {
    public:
        TitleBarButton (const juce::String& name, const juce::Path& normal, const juce::Path& toggled)
            : juce::Button (name), normalShape (normal), toggledShape (toggled) {}

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            if (over || down)   // soft highlight; the shape itself always stays bright
            {
                g.setColour (Palette::trace.withAlpha (down ? 0.32f : 0.18f));
                g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 3.0f);
            }
            auto& p = getToggleState() ? toggledShape : normalShape;
            const auto area = juce::Justification (juce::Justification::centred)
                                .appliedToRectangle (juce::Rectangle<int> (getHeight(), getHeight()), getLocalBounds())
                                .toFloat().reduced ((float) getHeight() * 0.3f);
            g.setColour (isEnabled() ? Palette::trace : Palette::trace.withAlpha (0.4f));
            g.fillPath (p, p.getTransformToScaleToFit (area, true));
        }

    private:
        juce::Path normalShape, toggledShape;
    };
}

juce::Button* WindowButtonsLookAndFeel::createDocumentWindowButton (int buttonType)
{
    juce::Path shape;
    const float t = 0.15f;
    if (buttonType == juce::DocumentWindow::closeButton)
    {
        shape.addLineSegment ({ 0.0f, 0.0f, 1.0f, 1.0f }, t);
        shape.addLineSegment ({ 1.0f, 0.0f, 0.0f, 1.0f }, t);
        return new TitleBarButton ("close", shape, shape);
    }
    if (buttonType == juce::DocumentWindow::minimiseButton)
    {
        shape.addLineSegment ({ 0.0f, 0.5f, 1.0f, 0.5f }, t);
        return new TitleBarButton ("minimise", shape, shape);
    }
    if (buttonType == juce::DocumentWindow::maximiseButton)
    {
        shape.addLineSegment ({ 0.5f, 0.0f, 0.5f, 1.0f }, t);
        shape.addLineSegment ({ 0.0f, 0.5f, 1.0f, 0.5f }, t);
        juce::Path full;
        full.addRectangle (0.0f, 0.0f, 1.0f, 1.0f);
        juce::PathStrokeType (0.15f).createStrokedPath (full, full);
        return new TitleBarButton ("maximise", shape, full);
    }
    return juce::LookAndFeel_V4::createDocumentWindowButton (buttonType);
}

void SpektreLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                           juce::Slider::SliderStyle, juce::Slider&)
{
    const float cy = (float) y + (float) h * 0.5f, x0 = (float) x, x1 = (float) (x + w);
    g.setColour (Palette::edge);
    g.fillRoundedRectangle (x0, cy - 2.0f, x1 - x0, 4.0f, 2.0f);
    g.setColour (Palette::trace);
    g.fillRoundedRectangle (x0, cy - 2.0f, pos - x0, 4.0f, 2.0f);
    g.fillEllipse (pos - 7.0f, cy - 7.0f, 14.0f, 14.0f);
}

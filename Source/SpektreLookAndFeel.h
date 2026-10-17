#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Palette and type from the web version
namespace Palette
{
    const juce::Colour bg     { 0xff0a0d13 };
    const juce::Colour panel  { 0xff10151e };
    const juce::Colour edge   { 0xff232b3a };
    const juce::Colour edgeHi { 0xff3a4558 };
    const juce::Colour fg     { 0xffdfe4ee };
    const juce::Colour muted  { 0xff7f8a9e };
    const juce::Colour trace  { 0xffffb547 };
    const juce::Colour hold   { 0xff7fd3ff };
    const juce::Colour warn   { 0xffff7a66 };
}

struct Fonts
{
    static juce::Font label (float h, bool bold = false);   // Barlow Condensed
    static juce::Font mono  (float h, bool semibold = false); // JetBrains Mono
};

class SpektreLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SpektreLookAndFeel();

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Font getLabelFont (juce::Label&) override;

    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
};

// Used only for the standalone app's window: the default title-bar buttons (dark red X,
// dark gold minimise) are hard to see on the dark title bar, so draw them in Spektre yellow.
class WindowButtonsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Button* createDocumentWindowButton (int buttonType) override;
};

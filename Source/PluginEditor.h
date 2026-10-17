#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Analyzer.h"
#include "SpectrumView.h"
#include "SpektreLookAndFeel.h"

class SpektreEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit SpektreEditor (SpektreProcessor&);
    ~SpektreEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void parentHierarchyChanged() override;

    void timerCallback() override;   // public so the test harness can drive frames

private:
    void settingsChanged();
    void addCombo (juce::ComboBox& box, juce::Label& label, const juce::String& title,
                   std::initializer_list<std::pair<int, juce::String>> items, int selected);

    SpektreProcessor& proc;
    SpektreLookAndFeel lnf;
    WindowButtonsLookAndFeel windowLnf;                       // standalone window only
    juce::Component::SafePointer<juce::DocumentWindow> hostWindow;
    Analyzer analyzer;
    SpectrumView view;
    Settings settings;

    juce::ComboBox fftBox, modeBox, slopeBox, peakBox, rangeBox, chanBox;
    juce::Label fftLabel, modeLabel, slopeLabel, peakLabel, rangeLabel, chanLabel, smoothLabel;
    juce::Slider smoothSlider;
    juce::TextButton sgButton { "Spectrogram" }, freezeButton { "Freeze" };

    struct Readout { juce::String title, value; bool key = false; };
    std::array<Readout, 6> readouts;
    juce::Rectangle<int> headerArea, readoutArea;

    uint32_t lastReset = 0;
    double lastTick = 0, lastReadout = 0;
    bool frozen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpektreEditor)
};

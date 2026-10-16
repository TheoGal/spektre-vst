#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Analyzer.h"

// The plot: log-frequency spectrum (line or octave bars), peak hold, cursor readout,
// and an optional scrolling spectrogram underneath.
class SpectrumView : public juce::Component
{
public:
    SpectrumView();

    void setSettings (const Settings& s);
    void setActive (bool hasSignal) { active = hasSignal; }
    void update (const Analyzer& a, float dtSeconds, bool frozen);   // after each analysis frame
    void resetHold();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    static juce::String fmtFreq (float f);
    static juce::String fmtDb (float d);
    static juce::String noteOf (float f);

private:
    struct Band { float fc, lo, hi; };

    void layoutPlot();
    void drawGrid (juce::Graphics&);
    void buildBands();
    float xOfF (float f) const;
    float fOfX (float x) const;
    float yOfDb (float d) const;
    float slopeAt (float f) const;
    float binAt (int i) const;
    float interp (float x) const;
    float sampleDb (float lo, float hi) const;
    void computeSeries (float dt);
    void drawSpectrogramColumn();
    juce::Colour sgColour (float t) const;

    Settings settings;
    bool active = false;

    // copy of the analyzer's bins for this frame
    std::vector<float> dbw;
    float binHz = 1, sampleRate = 48000, fmax = 20000;
    Analyzer::Peak peakInfo;

    // plot geometry
    juce::Rectangle<int> plotArea, sgArea;
    float pl = 42, pr = 10, pt = 10, pb = 22, pw = 1, ph = 1;
    int nPix = 2;
    juce::Image sgImage;
    std::vector<Band> bands;
    std::vector<float> disp, hold;
    std::vector<uint8_t> lowRes;
    juce::Colour sgLut[256];

    std::optional<float> cursorX;
};

#pragma once
#include <juce_dsp/juce_dsp.h>
#include "Settings.h"

// FFT analysis, ported 1:1 from the web version (which used Web Audio's AnalyserNode):
//  - Blackman window, a0 = 0.42, a1 = 0.5, a2 = 0.08, period N
//  - magnitude scaled by 1/N, exponential smoothing on the linear magnitude, then dB
//  - a full-scale sine therefore reads 20*log10(0.5*0.42) = -13.56 dB in its bin;
//    lineCal brings that back to 0 dBFS, bandCal does the same for power-summed bands.
// Runs on the message thread only.
class Analyzer
{
public:
    static constexpr int historySize = 32768;
    static inline const float lineCal = (float) (-20.0 * std::log10 (0.5 * 0.42));
    static inline const float bandCal = (float) (10.0 * std::log10 (4.0 / 0.3046));

    Analyzer();

    void setSampleRate (double sr);
    double getSampleRate() const { return sampleRate; }
    void reset();

    // append new input (any length)
    void push (const float* l, const float* r, int n);

    // run one analysis frame with the given settings
    void compute (const Settings& s);

    // results (valid after compute)
    int   numBins() const      { return (int) db.size(); }
    float binHz() const        { return binHzValue; }
    const std::vector<float>& bins() const { return db; }   // raw dB per bin (add lineCal for dBFS)

    struct Peak { float freq = 0, dbfs = -200; };
    Peak  peak (float fmin, float fmax) const;               // interpolated, calibrated dBFS
    float rmsDbfs()  const { return rmsDb; }                 // sine-referenced: a 0 dBFS sine reads 0
    float samplePeakDbfs() const { return peakDb; }

private:
    void prepare (int fftSize);
    float sampleChannel (int idx, int channel) const;

    double sampleRate = 48000.0;
    std::vector<float> histL, histR;
    int writePos = 0;

    int currentSize = 0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window, work, smoothMag, db;
    float binHzValue = 1.0f;
    float rmsDb = -200.0f, peakDb = -200.0f;
};

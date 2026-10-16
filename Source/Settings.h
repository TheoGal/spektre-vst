#pragma once
#include <juce_data_structures/juce_data_structures.h>

// Analyzer settings. Edited on the message thread by the editor and saved in the
// plugin state, so every instance in a project remembers its own view.
struct Settings
{
    int   fftSize   = 16384;  // 2048 .. 32768
    float smoothing = 0.5f;   // 0 .. 0.95, same meaning as Web Audio AnalyserNode
    int   mode      = 0;      // 0 = line, 3 / 6 / 12 = 1/n-octave bars
    float slope     = 4.5f;   // dB per octave, pivot 1 kHz (display only)
    int   peakHold  = 1;      // 0 off, 1 decay, 2 max hold
    int   floorDb   = -100;   // bottom of the plot
    int   channel   = 0;      // 0 L+R, 1 L, 2 R
    bool  spectrogram = false;

    juce::ValueTree toTree() const
    {
        juce::ValueTree t ("SPEKTRE");
        t.setProperty ("fft", fftSize, nullptr);
        t.setProperty ("smooth", smoothing, nullptr);
        t.setProperty ("mode", mode, nullptr);
        t.setProperty ("slope", slope, nullptr);
        t.setProperty ("peak", peakHold, nullptr);
        t.setProperty ("floor", floorDb, nullptr);
        t.setProperty ("chan", channel, nullptr);
        t.setProperty ("sg", spectrogram, nullptr);
        return t;
    }

    static Settings fromTree (const juce::ValueTree& t)
    {
        Settings s;
        if (! t.hasType ("SPEKTRE")) return s;
        s.fftSize     = juce::jlimit (2048, 32768, (int) t.getProperty ("fft", s.fftSize));
        if (! juce::isPowerOfTwo (s.fftSize)) s.fftSize = 16384;
        s.smoothing   = juce::jlimit (0.0f, 0.95f, (float) t.getProperty ("smooth", s.smoothing));
        s.mode        = (int) t.getProperty ("mode", s.mode);
        s.slope       = (float) t.getProperty ("slope", s.slope);
        s.peakHold    = juce::jlimit (0, 2, (int) t.getProperty ("peak", s.peakHold));
        s.floorDb     = juce::jlimit (-120, -60, (int) t.getProperty ("floor", s.floorDb));
        s.channel     = juce::jlimit (0, 2, (int) t.getProperty ("chan", s.channel));
        s.spectrogram = (bool) t.getProperty ("sg", s.spectrogram);
        return s;
    }
};

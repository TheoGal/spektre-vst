#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Settings.h"

// Lock-free hand-off of stereo samples from the audio thread to the UI.
// The audio thread only writes; the editor's timer only reads.
class SampleFifo
{
public:
    static constexpr int capacity = 1 << 17;

    SampleFifo() : buffer (2, capacity) { buffer.clear(); }

    void push (const float* l, const float* r, int n) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (n, s1, n1, s2, n2);   // drops samples if the UI isn't reading
        copy (l, r, 0, s1, n1);
        copy (l, r, n1, s2, n2);
        fifo.finishedWrite (n1 + n2);
    }

    template <typename Fn> void pop (Fn&& consume)  // consume (const float* l, const float* r, int n)
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
        if (n1 > 0) consume (buffer.getReadPointer (0, s1), buffer.getReadPointer (1, s1), n1);
        if (n2 > 0) consume (buffer.getReadPointer (0, s2), buffer.getReadPointer (1, s2), n2);
        fifo.finishedRead (n1 + n2);
    }

private:
    void copy (const float* l, const float* r, int srcOff, int dst, int n) noexcept
    {
        if (n <= 0) return;
        juce::FloatVectorOperations::copy (buffer.getWritePointer (0, dst), l + srcOff, n);
        juce::FloatVectorOperations::copy (buffer.getWritePointer (1, dst), r + srcOff, n);
    }

    juce::AbstractFifo fifo { capacity };
    juce::AudioBuffer<float> buffer;
};

class SpektreProcessor : public juce::AudioProcessor
{
public:
    SpektreProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    Settings getSettings() const            { const juce::SpinLock::ScopedLockType l (settingsLock); return settings; }
    void setSettings (const Settings& s)    { const juce::SpinLock::ScopedLockType l (settingsLock); settings = s; }

    SampleFifo fifo;
    std::atomic<double> sampleRate { 48000.0 };
    std::atomic<int>    numInputChannels { 2 };
    std::atomic<uint32_t> resetCounter { 0 };   // bumped when the stream restarts

private:
    Settings settings;
    juce::SpinLock settingsLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpektreProcessor)
};

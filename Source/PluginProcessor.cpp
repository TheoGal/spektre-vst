#include "PluginProcessor.h"
#include "PluginEditor.h"

SpektreProcessor::SpektreProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

bool SpektreProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void SpektreProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    numInputChannels = getTotalNumInputChannels();
    ++resetCounter;
}

void SpektreProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nIn = getTotalNumInputChannels();

    // audio passes through untouched; we only take a copy for the analyzer
    for (int ch = nIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    if (n == 0 || nIn == 0) return;
    const float* l = buffer.getReadPointer (0);
    const float* r = nIn > 1 ? buffer.getReadPointer (1) : l;
    fifo.push (l, r, n);
}

juce::AudioProcessorEditor* SpektreProcessor::createEditor()
{
    return new SpektreEditor (*this);
}

void SpektreProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = getSettings().toTree().createXml())
        copyXmlToBinary (*xml, dest);
}

void SpektreProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        setSettings (Settings::fromTree (juce::ValueTree::fromXml (*xml)));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpektreProcessor();
}

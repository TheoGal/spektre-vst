#include "PluginProcessor.h"
#include "PluginEditor.h"

static bool isStandalone()
{
    return juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone;
}

// In a DAW the plugin sits in the signal chain, so it needs an output and passes audio
// through untouched. The standalone app only listens: it has no output at all, so it
// can never feed the input back to the speakers.
juce::AudioProcessor::BusesProperties SpektreProcessor::makeBuses()
{
    auto buses = BusesProperties()
                   .withInput ("Input", juce::AudioChannelSet::stereo(), true);
    if (! isStandalone())
        buses = buses.withOutput ("Output", juce::AudioChannelSet::stereo(), true);
    return buses;
}

SpektreProcessor::SpektreProcessor() : AudioProcessor (makeBuses())
{
}

bool SpektreProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    if (isStandalone()) return out.isDisabled();   // listen-only
    return out == in;
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

    if (wrapperType == wrapperType_Standalone)   // belt and braces: never output anything
        buffer.clear();
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

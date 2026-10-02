#pragma once

#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

// Drives the real processor the way a host does, for tests and measurements that need no host.
namespace ProcessorHarness
{
    inline void setParameter (juce::AudioProcessor& processor, const juce::String& parameterId, float value)
    {
        for (auto* parameter : processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter); ranged != nullptr && ranged->getParameterID() == parameterId)
                ranged->setValueNotifyingHost (ranged->convertTo0to1 (value));
    }

    inline void useChannelCount (juce::AudioProcessor& processor, int numChannels)
    {
        const auto channelSet = juce::AudioChannelSet::canonicalChannelSet (numChannels);
        juce::AudioProcessor::BusesLayout layout;
        layout.inputBuses.add (channelSet);
        layout.outputBuses.add (channelSet);
        processor.setBusesLayout (layout);
    }

    // Renders in place, block by block. beforeBlock lets a test change parameters or pick the bypassed path.
    inline void render (BigCityAudioProcessor& processor, juce::AudioBuffer<float>& audio, int blockSize,
                        const std::function<bool (int blockIndex)>& beforeBlock = {})
    {
        juce::MidiBuffer midi;

        for (int start = 0, blockIndex = 0; start < audio.getNumSamples(); start += blockSize, ++blockIndex)
        {
            const auto length = std::min (blockSize, audio.getNumSamples() - start);
            juce::AudioBuffer<float> block (audio.getArrayOfWritePointers(), audio.getNumChannels(), start, length);
            const auto useBypassedPath = beforeBlock ? beforeBlock (blockIndex) : false;

            if (useBypassedPath)
                processor.processBlockBypassed (block, midi);
            else
                processor.processBlock (block, midi);
        }
    }
}

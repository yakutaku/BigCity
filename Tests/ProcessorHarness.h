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

    // Picks the voicing the only way a host can: by restoring a saved state that names it.
    inline void useVoicing (juce::AudioProcessor& processor, Voicing voicing)
    {
        juce::MemoryBlock state;
        processor.getStateInformation (state);

        auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
        xml->setAttribute ("voicing", (int) voicing);

        juce::MemoryBlock edited;
        juce::AudioProcessor::copyXmlToBinary (*xml, edited);
        processor.setStateInformation (edited.getData(), (int) edited.getSize());
    }

    struct BlockPosition
    {
        int index, startSample;
    };

    using BlockSizeSchedule = std::function<int (int blockIndex)>;
    using BeforeBlock = std::function<bool (BlockPosition)>;

    // Renders in place, block by block, with a block size that may change from call to call as in a host.
    // beforeBlock lets a test change parameters or pick the bypassed path.
    inline void render (BigCityAudioProcessor& processor, juce::AudioBuffer<float>& audio,
                        const BlockSizeSchedule& blockSizeAt, const BeforeBlock& beforeBlock = {})
    {
        juce::MidiBuffer midi;

        for (int start = 0, blockIndex = 0; start < audio.getNumSamples(); start += blockSizeAt (blockIndex), ++blockIndex)
        {
            const auto length = std::min (blockSizeAt (blockIndex), audio.getNumSamples() - start);
            juce::AudioBuffer<float> block (audio.getArrayOfWritePointers(), audio.getNumChannels(), start, length);
            const auto useBypassedPath = beforeBlock ? beforeBlock ({ blockIndex, start }) : false;

            if (useBypassedPath)
                processor.processBlockBypassed (block, midi);
            else
                processor.processBlock (block, midi);
        }
    }

    inline void render (BigCityAudioProcessor& processor, juce::AudioBuffer<float>& audio, int blockSize,
                        const BeforeBlock& beforeBlock = {})
    {
        render (processor, audio, [blockSize] (int) { return blockSize; }, beforeBlock);
    }
}

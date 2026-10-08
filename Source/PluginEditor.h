#pragma once

#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The fixed-size BigCity window. It draws the static panel and lays out one component per control.
class BigCityAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BigCityAudioProcessorEditor (BigCityAudioProcessor& bigCityProcessor);

    void paint (juce::Graphics& g) override;

private:
    // Decoded once; the panel never changes, so every repaint just redraws this image.
    const juce::Image panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigCityAudioProcessorEditor)
};

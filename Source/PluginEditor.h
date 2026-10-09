#pragma once

#include "CityKnob.h"
#include "PluginProcessor.h"
#include "SevenSegmentReadout.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The fixed-size BigCity window. It draws the static panel and lays out one component per control.
class BigCityAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit BigCityAudioProcessorEditor (BigCityAudioProcessor& bigCityProcessor);

    void paint (juce::Graphics& g) override;

private:
    // Polls citySize for the readout, so it repaints at no more than the spec's rate. The knob follows
    // the display on its own (D-25).
    void timerCallback() override;

    juce::AudioParameterFloat& citySize;

    // Decoded once; the panel never changes, so every repaint just redraws this image.
    const juce::Image panel;

    CityKnob knob;
    SevenSegmentReadout readout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigCityAudioProcessorEditor)
};

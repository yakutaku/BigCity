#pragma once

#include "BypassSwitch.h"
#include "CityKnob.h"
#include "PluginProcessor.h"
#include "SevenSegmentReadout.h"
#include "Skyline.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The fixed-size BigCity window. It draws the static panel and lays out one component per control.
class BigCityAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit BigCityAudioProcessorEditor (BigCityAudioProcessor& bigCityProcessor);

    void paint (juce::Graphics& g) override;

private:
    // Polls citySize and bypass for the readout, skyline and switch, so they repaint at no more than the spec's
    // rate. The knob follows the display on its own (D-25).
    void timerCallback() override;
    bool isBypassed() const;

    juce::AudioParameterFloat& citySize;
    juce::AudioProcessorParameter& bypass;

    // Decoded once; the panel never changes, so every repaint just redraws this image.
    const juce::Image panel;

    CityKnob knob;
    SevenSegmentReadout readout;
    Skyline skyline;
    BypassSwitch bypassSwitch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigCityAudioProcessorEditor)
};

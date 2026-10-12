#pragma once

#include "BypassSwitch.h"
#include "CityKnob.h"
#include "PluginProcessor.h"
#include "SevenSegmentReadout.h"
#include "Skyline.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The fixed-size BigCity window. It draws the static panel and lays out one component per control.
class BigCityAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BigCityAudioProcessorEditor (BigCityAudioProcessor& bigCityProcessor);

    void paint (juce::Graphics& g) override;

private:
    // One display-synced tick for every control, so their repaints share a frame (D-25): the knob on every
    // tick, the readout, skyline and switch on every other one.
    void tick (double timestampSeconds);
    void updateDisplays();
    bool isBypassed() const;

    juce::AudioParameterFloat& citySize;
    juce::AudioProcessorParameter& bypass;

    // Decoded once; the panel never changes, so every repaint just redraws this image.
    const juce::Image panel;

    CityKnob knob;
    SevenSegmentReadout readout;
    Skyline skyline;
    BypassSwitch bypassSwitch;

    double lastTickSeconds = 0.0;
    int tickCount = 0;

    // Last, so it can't tick before everything it drives exists.
    juce::VBlankAttachment displayRefresh;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigCityAudioProcessorEditor)
};

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Counts gesture starts and ends, so a test can check every begin has its end.
struct GestureCounter final : private juce::AudioProcessorParameter::Listener
{
    explicit GestureCounter (juce::AudioProcessorParameter& parameterToWatch) : parameter (parameterToWatch)
    {
        parameter.addListener (this);
    }

    ~GestureCounter() override { parameter.removeListener (this); }

    int balance() const { return begins - ends; }

    int begins = 0, ends = 0;

private:
    void parameterValueChanged (int, float) override {}
    void parameterGestureChanged (int, bool starting) override { ++(starting ? begins : ends); }

    juce::AudioProcessorParameter& parameter;
};

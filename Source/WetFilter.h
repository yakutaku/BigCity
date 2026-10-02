#pragma once

#include "Biquad.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>

// Band-limits the micro-shift's wet signal: a 12 dB/oct Butterworth high-pass and low-pass per
// channel, run as two calls so other stages can sit between them. Cutoffs glide rather than jump,
// so moving them while audio plays never clicks.
class WetFilter
{
public:
    static constexpr int maxChannels = 2;

    struct Cutoffs
    {
        double highPassHz, lowPassHz;
    };

    // Glides to these cutoffs. Set them before prepare to start there instead.
    void setCutoffs (Cutoffs targets);

    // Jumps to the target cutoffs and clears the filter state.
    void prepare (double newSampleRate);

    // Moves the glide on by numSamples and recalculates the coefficients once for that span.
    void advance (int numSamples);

    Cutoffs currentCutoffs() const;

    float processHighPass (int channel, float input)
    {
        return channelStates[(size_t) channel].highPass.process (highPassCoefficients, input);
    }

    float processLowPass (int channel, float input)
    {
        return channelStates[(size_t) channel].lowPass.process (lowPassCoefficients, input);
    }

private:
    void updateCoefficients();

    struct ChannelState
    {
        BiquadState highPass, lowPass;
    };

    // Multiplicative, so a glide sounds even in octaves rather than racing through the low end.
    using CutoffSmoother = juce::SmoothedValue<double, juce::ValueSmoothingTypes::Multiplicative>;

    double sampleRate = 44100.0;
    CutoffSmoother highPassHz, lowPassHz;
    BiquadCoefficients highPassCoefficients { 1.0, 0.0, 0.0, 0.0, 0.0 };
    BiquadCoefficients lowPassCoefficients { 1.0, 0.0, 0.0, 0.0, 0.0 };
    std::array<ChannelState, maxChannels> channelStates {};
};

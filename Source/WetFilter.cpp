#include "WetFilter.h"
#include "SoundSpec.h"

#include <juce_dsp/juce_dsp.h>

namespace
{
    // Maximally flat passband: the filters shape the wet signal's band without colouring inside it.
    constexpr double butterworthQ = juce::MathConstants<double>::sqrt2 / 2.0;
}

void WetFilter::setCutoffs (Cutoffs targets)
{
    highPassHz.setTargetValue (targets.highPassHz);
    lowPassHz.setTargetValue (targets.lowPassHz);
}

void WetFilter::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    // Same glide time as City Size, so every control responds alike.
    highPassHz.reset (sampleRate, SoundSpec::citySizeSmoothingSeconds);
    lowPassHz.reset (sampleRate, SoundSpec::citySizeSmoothingSeconds);

    updateCoefficients();
    channelStates.fill ({});
}

void WetFilter::advance (int numSamples)
{
    if (! highPassHz.isSmoothing() && ! lowPassHz.isSmoothing())
        return;

    highPassHz.skip (numSamples);
    lowPassHz.skip (numSamples);
    updateCoefficients();
}

WetFilter::Cutoffs WetFilter::currentCutoffs() const
{
    return { highPassHz.getCurrentValue(), lowPassHz.getCurrentValue() };
}

void WetFilter::updateCoefficients()
{
    // ArrayCoefficients return by value, so this is safe on the audio thread.
    using Design = juce::dsp::IIR::ArrayCoefficients<double>;
    highPassCoefficients = BiquadCoefficients::normalise (Design::makeHighPass (sampleRate, highPassHz.getCurrentValue(), butterworthQ));
    lowPassCoefficients = BiquadCoefficients::normalise (Design::makeLowPass (sampleRate, lowPassHz.getCurrentValue(), butterworthQ));
}

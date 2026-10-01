#include "AirShelf.h"

#include <juce_dsp/juce_dsp.h>

BiquadCoefficients AirShelf::makeCoefficients (double sampleRate, double cornerHz, double q, float gainDb)
{
    // JUCE's ArrayCoefficients follow the RBJ Audio EQ Cookbook and return by value, so this
    // is safe on the audio thread.
    const auto gain = (double) juce::Decibels::decibelsToGain (gainDb);
    const auto raw = juce::dsp::IIR::ArrayCoefficients<double>::makeHighShelf (sampleRate, cornerHz, q, gain);
    const auto a0 = raw[3];

    return { raw[0] / a0, raw[1] / a0, raw[2] / a0, raw[4] / a0, raw[5] / a0 };
}

void AirShelf::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    update (0.0f, SoundSpec::airCornerHz, SoundSpec::airShelfQ);
    reset();
}

void AirShelf::reset()
{
    channelStates.fill ({});
}

void AirShelf::update (float gainDb, double cornerHz, double q)
{
    coefficients = makeCoefficients (sampleRate, cornerHz, q, gainDb);
}

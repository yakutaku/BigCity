#pragma once

#include "Biquad.h"
#include "ProcessorHarness.h"

#include <cmath>

// The PRD §6 "Auto gain" measurement: pink noise through the real processor, K-weighted loudness
// compared with bypass. Shared by the AutoGainMeasure tool, which builds the curve, and the test
// that checks it.
namespace LoudnessMeasurement
{
    // PRD §6 "Auto gain" basis
    constexpr double sampleRate = 48000.0;
    constexpr float noiseRmsDbfs = -18.0f;

    // Long enough for the loudness of noise to be steady to well under 0.1 dB.
    constexpr int settleSamples = 48000;
    constexpr int measuredSamples = 10 * 48000;
    constexpr int blockSize = 512;

    // Paul Kellet's refined pink filter (musicdsp.org): seven one-pole sections whose sum stays
    // within about 0.05 dB of a 1/f slope across the audio band. The level is set afterwards.
    inline juce::AudioBuffer<float> makePinkNoise (int numChannels, int numSamples)
    {
        juce::AudioBuffer<float> noise (numChannels, numSamples);
        juce::Random random (1770);
        double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto white = (double) random.nextFloat() * 2.0 - 1.0;
            b0 = 0.99886 * b0 + white * 0.0555179;
            b1 = 0.99332 * b1 + white * 0.0750759;
            b2 = 0.96900 * b2 + white * 0.1538520;
            b3 = 0.86650 * b3 + white * 0.3104856;
            b4 = 0.55000 * b4 + white * 0.5329522;
            b5 = -0.7616 * b5 - white * 0.0168980;
            const auto pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362;
            b6 = white * 0.115926;

            for (int channel = 0; channel < numChannels; ++channel)
                noise.setSample (channel, i, (float) pink);
        }

        const auto rms = noise.getRMSLevel (0, 0, numSamples);
        noise.applyGain (juce::Decibels::decibelsToGain (noiseRmsDbfs) / rms);
        return noise;
    }

    // ITU-R BS.1770-4 loudness at 48 kHz, in LUFS. Ungated: on steady noise neither gate removes anything.
    inline double kWeightedLufs (const juce::AudioBuffer<float>& audio, int fromSample)
    {
        // BS.1770-4 Tables 1 and 2: the head-effect shelf, then the RLB high-pass, both specified at 48 kHz.
        constexpr BiquadCoefficients headShelf { 1.53512485958697, -2.69169618940638, 1.19839281085285, -1.69065929318241, 0.73248077421585 };
        constexpr BiquadCoefficients rlbHighPass { 1.0, -2.0, 1.0, -1.99004745483398, 0.99007225036621 };
        constexpr double loudnessOffsetDb = -0.691;

        double weightedPowerSum = 0.0;

        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        {
            BiquadState shelfState, highPassState;
            double sumOfSquares = 0.0;

            for (int i = 0; i < audio.getNumSamples(); ++i)
            {
                const auto weighted = highPassState.process (rlbHighPass, shelfState.process (headShelf, audio.getSample (channel, i)));

                if (i >= fromSample)
                    sumOfSquares += (double) weighted * weighted;
            }

            // Left and right both weigh 1.0 in BS.1770.
            weightedPowerSum += sumOfSquares / (audio.getNumSamples() - fromSample);
        }

        return loudnessOffsetDb + 10.0 * std::log10 (weightedPowerSum);
    }

    // Loudness through the plugin at this City Size minus the loudness of bypass, in dB.
    inline double differenceFromBypassDb (float citySizePercent, int numChannels)
    {
        BigCityAudioProcessor processor;
        ProcessorHarness::useChannelCount (processor, numChannels);
        ProcessorHarness::setParameter (processor, "citySize", citySizePercent);
        processor.prepareToPlay (sampleRate, blockSize);

        const auto input = makePinkNoise (numChannels, settleSamples + measuredSamples);
        auto output = input;
        ProcessorHarness::render (processor, output, blockSize);

        return kWeightedLufs (output, settleSamples) - kWeightedLufs (input, settleSamples);
    }
}

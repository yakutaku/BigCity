#pragma once

#include "Biquad.h"
#include "ProcessorHarness.h"

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cmath>
#include <complex>
#include <random>
#include <vector>

// The PRD §6 "Auto gain" measurement: programme simulation noise through the real processor,
// K-weighted loudness compared with bypass. Shared by the AutoGainMeasure tool, which builds the
// curve, and the test that checks it.
namespace LoudnessMeasurement
{
    // PRD §6 "Auto gain" basis
    constexpr double sampleRate = 48000.0;
    constexpr float noiseRmsDbfs = -18.0f;

    // Long enough for the loudness of noise to be steady to well under 0.1 dB.
    constexpr int settleSamples = 48000;
    constexpr int measuredSamples = 10 * 48000;
    constexpr int blockSize = 512;

    // IEC 60268-1:1985 Clause 7 "simulated programme signal", Table II: the relative power of each
    // one-third-octave band from 20 Hz to 20 kHz. It stands in for the average spectrum of speech and
    // music, which has much less top end than pink noise, so the air shelf is measured the way it
    // meets real material.
    constexpr std::array<double, 31> programmeBandLevelsDb {
        -13.5, -10.2, -7.4, -5.2, -3.5, -2.3, -1.4, -0.9, -0.5, -0.2, -0.1,   // 20 … 200 Hz
          0.0,   0.0,  0.0,  0.0,  0.0,  0.0,                                 // 250 … 800 Hz
         -0.1,  -0.3, -0.6, -1.0, -1.6, -2.5, -3.7, -5.1, -7.0, -9.4, -11.9,  // 1 … 10 kHz
        -14.8, -18.2, -21.6                                                   // 12.5 … 20 kHz
    };
    constexpr double oneKilohertzBand = 17;

    // Table II between the band centres: linear in dB against log frequency, carried on at the end
    // slopes beyond 20 Hz and 20 kHz. Every band's power then lands within 0.12 dB of its printed
    // level, well inside the table's tolerances (0.5 dB at the tightest).
    inline double programmeLevelDb (double frequencyHz)
    {
        constexpr auto lastSegment = (int) programmeBandLevelsDb.size() - 2;
        const auto bandPosition = oneKilohertzBand + 3.0 * std::log2 (frequencyHz / 1000.0);
        const auto lower = (size_t) juce::jlimit (0, lastSegment, (int) std::floor (bandPosition));
        const auto fraction = bandPosition - (double) lower;
        return programmeBandLevelsDb[lower] + fraction * (programmeBandLevelsDb[lower + 1] - programmeBandLevelsDb[lower]);
    }

    // Stationary Gaussian noise with the Table II spectrum, as the clause asks, made in the frequency
    // domain so the shape is exact: Gaussian bins weighted to pink (equal power per band) times
    // Table II, then one inverse FFT. The level is set afterwards.
    inline juce::AudioBuffer<float> makeProgrammeNoise (int numChannels, int numSamples)
    {
        juce::dsp::FFT fft ((int) std::ceil (std::log2 ((double) numSamples)));
        const auto fftSize = (size_t) fft.getSize();
        std::vector<std::complex<float>> spectrum (fftSize), signal (fftSize);
        std::mt19937 random (1770);
        std::normal_distribution<float> gaussian;

        for (size_t bin = 1; bin < fftSize / 2; ++bin)
        {
            const auto frequencyHz = (double) bin * sampleRate / (double) fftSize;
            const auto amplitude = (float) (juce::Decibels::decibelsToGain (programmeLevelDb (frequencyHz), -1000.0) / std::sqrt (frequencyHz));
            spectrum[bin] = amplitude * std::complex<float> (gaussian (random), gaussian (random));
            spectrum[fftSize - bin] = std::conj (spectrum[bin]);
        }

        fft.perform (spectrum.data(), signal.data(), true);

        juce::AudioBuffer<float> noise (numChannels, numSamples);

        for (int channel = 0; channel < numChannels; ++channel)
            for (int i = 0; i < numSamples; ++i)
                noise.setSample (channel, i, signal[(size_t) i].real());

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
    inline double differenceFromBypassDb (float citySizePercent, int numChannels, Voicing voicing)
    {
        BigCityAudioProcessor processor;
        ProcessorHarness::useVoicing (processor, voicing);
        ProcessorHarness::useChannelCount (processor, numChannels);
        ProcessorHarness::setParameter (processor, "citySize", citySizePercent);
        processor.prepareToPlay (sampleRate, blockSize);

        const auto input = makeProgrammeNoise (numChannels, settleSamples + measuredSamples);
        auto output = input;
        ProcessorHarness::render (processor, output, blockSize);

        return kWeightedLufs (output, settleSamples) - kWeightedLufs (input, settleSamples);
    }
}

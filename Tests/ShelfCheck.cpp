// PRD §6 "Shelf response check (Day 3)": prints the air shelf's response at City Size 100 for
// 44.1, 48 and 96 kHz, at Q 0.5 and Q 0.707, and flags any spread over 0.5 dB below 16 kHz.
// A flag is a finding to discuss, not a failure, so this always exits 0.

#include "AirShelf.h"
#include "CityMacro.h"
#include "SoundSpec.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdio>

namespace
{
    constexpr std::array<double, 3> sampleRatesHz { 44100.0, 48000.0, 96000.0 };
    constexpr std::array<double, 9> reportFrequenciesHz { 1000.0, 2000.0, 4000.0, 6000.0, 8000.0, 10000.0, 12000.0, 14000.0, 16000.0 };

    // From PRD §6 "Shelf response check (Day 3)"
    constexpr double checkUpToHz = 16000.0;
    constexpr double flagThresholdDb = 0.5;

    constexpr double scanLowestHz = 20.0;
    constexpr double scanStepHz = 10.0;

    double magnitudeDb (const BiquadCoefficients& c, double frequencyHz, double sampleRate)
    {
        const auto omega = juce::MathConstants<double>::twoPi * frequencyHz / sampleRate;
        const auto z1 = std::polar (1.0, -omega);
        const auto z2 = z1 * z1;
        const auto response = (c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2);
        return 20.0 * std::log10 (std::abs (response));
    }

    double spreadAcrossSampleRates (double q, float gainDb, double frequencyHz)
    {
        double lowestDb = 1.0e9, highestDb = -1.0e9;

        for (auto sampleRate : sampleRatesHz)
        {
            const auto db = magnitudeDb (AirShelf::makeCoefficients (sampleRate, SoundSpec::airCornerHz, q, gainDb), frequencyHz, sampleRate);
            lowestDb = std::min (lowestDb, db);
            highestDb = std::max (highestDb, db);
        }

        return highestDb - lowestDb;
    }

    void printTable (double q, float gainDb)
    {
        std::printf ("\nQ %.3f, gain %+.1f dB\n", q, (double) gainDb);
        std::printf ("%10s %10s %10s %10s %10s\n", "Hz", "44.1 kHz", "48 kHz", "96 kHz", "spread");

        for (auto frequencyHz : reportFrequenciesHz)
        {
            std::printf ("%10.0f", frequencyHz);

            for (auto sampleRate : sampleRatesHz)
                std::printf (" %+10.2f", magnitudeDb (AirShelf::makeCoefficients (sampleRate, SoundSpec::airCornerHz, q, gainDb), frequencyHz, sampleRate));

            std::printf (" %10.2f\n", spreadAcrossSampleRates (q, gainDb, frequencyHz));
        }
    }

    void reportWorstSpread (double q, float gainDb)
    {
        double worstSpreadDb = 0.0, worstFrequencyHz = 0.0, firstFlagHz = 0.0;

        for (auto frequencyHz = scanLowestHz; frequencyHz <= checkUpToHz; frequencyHz += scanStepHz)
        {
            const auto spreadDb = spreadAcrossSampleRates (q, gainDb, frequencyHz);

            if (spreadDb > flagThresholdDb && firstFlagHz == 0.0)
                firstFlagHz = frequencyHz;

            if (spreadDb > worstSpreadDb)
            {
                worstSpreadDb = spreadDb;
                worstFrequencyHz = frequencyHz;
            }
        }

        if (firstFlagHz > 0.0)
            std::printf ("FLAG: Q %.3f spread exceeds %.1f dB from %.0f Hz; worst %.2f dB at %.0f Hz\n",
                         q, flagThresholdDb, firstFlagHz, worstSpreadDb, worstFrequencyHz);
        else
            std::printf ("OK:   Q %.3f worst spread %.2f dB at %.0f Hz (limit %.1f dB below %.0f Hz)\n",
                         q, worstSpreadDb, worstFrequencyHz, flagThresholdDb, checkUpToHz);
    }
}

int main()
{
    const auto gainDb = CityMacro::map (SoundSpec::citySizeMaxPercent, Voicing::original).airGainDb;

    for (auto q : { SoundSpec::airShelfQ, SoundSpec::airShelfAlternativeQ })
        printTable (q, gainDb);

    std::printf ("\n");

    for (auto q : { SoundSpec::airShelfQ, SoundSpec::airShelfAlternativeQ })
        reportWorstSpread (q, gainDb);

    return 0;
}

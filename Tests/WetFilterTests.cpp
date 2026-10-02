#include "WetFilter.h"

#include <juce_core/juce_core.h>

#include <cmath>

class WetFilterTests final : public juce::UnitTest
{
public:
    WetFilterTests() : juce::UnitTest ("WetFilter", "BigCity") {}

    void runTest() override
    {
        testPassband();
        testCutoffsGlide();
    }

private:
    static constexpr double sampleRate = 48000.0;
    static constexpr WetFilter::Cutoffs tuningStartCutoffs { 250.0, 8000.0 };

    // Steady-state gain of a sine through a freshly prepared filter, in dB.
    static double gainDbAt (double frequencyHz)
    {
        WetFilter filter;
        filter.setCutoffs (tuningStartCutoffs);
        filter.prepare (sampleRate);

        constexpr int settleSamples = 48000;
        constexpr int measuredSamples = 48000;
        float peak = 0.0f;

        for (int i = 0; i < settleSamples + measuredSamples; ++i)
        {
            const auto input = (float) std::sin (juce::MathConstants<double>::twoPi * frequencyHz * i / sampleRate);
            const auto output = filter.processLowPass (0, filter.processHighPass (0, input));

            if (i >= settleSamples)
                peak = std::max (peak, std::abs (output));
        }

        return juce::Decibels::gainToDecibels ((double) peak);
    }

    void testPassband()
    {
        beginTest ("High-pass 250 Hz and low-pass 8 kHz cut 50 Hz and 15 kHz and pass 1 kHz");

        logMessage ("  50 Hz " + juce::String (gainDbAt (50.0), 2) + " dB, 1 kHz " + juce::String (gainDbAt (1000.0), 2)
                    + " dB, 15 kHz " + juce::String (gainDbAt (15000.0), 2) + " dB");

        // 12 dB/oct: 50 Hz is over two octaves below the high-pass, 15 kHz nearly one above the low-pass.
        expectLessThan (gainDbAt (50.0), -24.0);
        expectWithinAbsoluteError (gainDbAt (1000.0), 0.0, 0.25);
        expectLessThan (gainDbAt (15000.0), -9.0);
    }

    void testCutoffsGlide()
    {
        beginTest ("A cutoff change glides over about 30 ms instead of jumping");

        WetFilter filter;
        filter.setCutoffs (tuningStartCutoffs);
        filter.prepare (sampleRate);

        filter.setCutoffs ({ 1000.0, 4000.0 });
        filter.advance (32);
        const auto afterOneChunk = filter.currentCutoffs();
        expect (afterOneChunk.highPassHz > 250.0 && afterOneChunk.highPassHz < 300.0, "high-pass jumped to " + juce::String (afterOneChunk.highPassHz));
        expect (afterOneChunk.lowPassHz < 8000.0 && afterOneChunk.lowPassHz > 7000.0, "low-pass jumped to " + juce::String (afterOneChunk.lowPassHz));

        filter.advance ((int) (sampleRate * 0.030));
        const auto afterGlide = filter.currentCutoffs();
        expectWithinAbsoluteError (afterGlide.highPassHz, 1000.0, 1.0e-6);
        expectWithinAbsoluteError (afterGlide.lowPassHz, 4000.0, 1.0e-6);
    }
};

static WetFilterTests wetFilterTests;

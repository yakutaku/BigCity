#include "MicroShifter.h"

#include <juce_core/juce_core.h>

#include <cmath>

class MicroShifterTests final : public juce::UnitTest
{
public:
    MicroShifterTests() : juce::UnitTest ("MicroShifter", "BigCity") {}

    void runTest() override
    {
        testDetune();
        testSweepIsOppositePhaseAndUpwardOnly();
        testZeroDepthLeavesTheShiftUnchanged();
        testDeepestFastestModulationStaysBounded();
    }

private:
    static constexpr double sampleRate = 48000.0;

    static float sineAt (int sampleIndex, double frequencyHz)
    {
        return (float) std::sin (juce::MathConstants<double>::twoPi * frequencyHz * sampleIndex / sampleRate);
    }

    void testDetune()
    {
        constexpr double inputHz = 1000.0;
        constexpr float detuneCents = 10.0f;
        constexpr double toleranceCents = 1.0;

        // Long enough to cover two full crossfade cycles at 10 cents, so tap switching averages out.
        constexpr int warmUpSamples = 4800;
        constexpr int measuredSamples = 2 * 166000;

        MicroShifter shifter;
        shifter.prepare (sampleRate);
        shifter.setDetuneCents (detuneCents);

        int leftCrossings = 0, rightCrossings = 0;
        MicroShifter::StereoSample previous { 0.0f, 0.0f };

        for (int i = 0; i < warmUpSamples + measuredSamples; ++i)
        {
            const auto input = (float) std::sin (juce::MathConstants<double>::twoPi * inputHz * i / sampleRate);
            const auto output = shifter.process (input, input);

            if (i > warmUpSamples)
            {
                leftCrossings += (previous.left <= 0.0f && output.left > 0.0f) ? 1 : 0;
                rightCrossings += (previous.right <= 0.0f && output.right > 0.0f) ? 1 : 0;
            }

            previous = output;
        }

        const auto measuredSeconds = measuredSamples / sampleRate;
        const auto centsFromInput = [&] (int crossings) { return 1200.0 * std::log2 (crossings / measuredSeconds / inputHz); };

        beginTest ("Left voice is detuned down by the requested cents");
        expectWithinAbsoluteError (centsFromInput (leftCrossings), -(double) detuneCents, toleranceCents);

        beginTest ("Right voice is detuned up by the requested cents");
        expectWithinAbsoluteError (centsFromInput (rightCrossings), (double) detuneCents, toleranceCents);
    }

    void testSweepIsOppositePhaseAndUpwardOnly()
    {
        beginTest ("The sweep moves left and right in opposite phase, between 0 and the depth");

        constexpr double depthSamples = 72.0;

        for (auto lfoValue : { -1.0, -0.5, 0.0, 0.5, 1.0 })
        {
            const auto sweep = MicroShifter::sweepFor (lfoValue, depthSamples);
            expectWithinAbsoluteError (sweep.leftSamples + sweep.rightSamples, depthSamples, 1.0e-9);
            expect (sweep.leftSamples >= 0.0 && sweep.leftSamples <= depthSamples, "left sweep out of range");
            expect (sweep.rightSamples >= 0.0 && sweep.rightSamples <= depthSamples, "right sweep out of range");
        }

        expectEquals (MicroShifter::sweepFor (1.0, depthSamples).leftSamples, depthSamples);
        expectEquals (MicroShifter::sweepFor (1.0, depthSamples).rightSamples, 0.0);
        expectEquals (MicroShifter::sweepFor (-1.0, depthSamples).leftSamples, 0.0);
        expectEquals (MicroShifter::sweepFor (-1.0, depthSamples).rightSamples, depthSamples);
    }

    void testZeroDepthLeavesTheShiftUnchanged()
    {
        beginTest ("Modulation at 0 ms depth is bit-identical to no modulation, as in shipping builds");

        MicroShifter unmodulated, zeroDepth;
        zeroDepth.setModulation ({ 2.0, 0.0 });

        for (auto* shifter : { &unmodulated, &zeroDepth })
        {
            shifter->prepare (sampleRate);
            shifter->setDetuneCents (10.0f);
        }

        auto identical = true;

        for (int i = 0; i < (int) sampleRate; ++i)
        {
            const auto input = sineAt (i, 440.0);
            const auto a = unmodulated.process (input, input);
            const auto b = zeroDepth.process (input, input);
            identical = identical && juce::exactlyEqual (a.left, b.left) && juce::exactlyEqual (a.right, b.right);
        }

        expect (identical, "zero-depth modulation changed the output");
    }

    void testDeepestFastestModulationStaysBounded()
    {
        beginTest ("At the deepest, fastest modulation the output stays bounded and differs from unmodulated");

        MicroShifter unmodulated, modulated;
        modulated.setModulation ({ 2.0, MicroShifter::maxModulationDepthMs });

        for (auto* shifter : { &unmodulated, &modulated })
        {
            shifter->prepare (sampleRate);
            shifter->setDetuneCents (10.0f);
        }

        // The fades sum to 1, so the output stays at the input's level; the margin allows interpolation overshoot.
        constexpr float peakLimit = 1.01f;
        float peak = 0.0f, largestDifference = 0.0f;

        for (int i = 0; i < 2 * (int) sampleRate; ++i)
        {
            const auto input = sineAt (i, 440.0);
            const auto a = unmodulated.process (input, input);
            const auto b = modulated.process (input, input);
            peak = std::max ({ peak, std::abs (b.left), std::abs (b.right) });
            largestDifference = std::max ({ largestDifference, std::abs (a.left - b.left), std::abs (a.right - b.right) });
        }

        expect (peak <= peakLimit, "peak " + juce::String (peak));
        expect (largestDifference > 0.1f, "modulation made little difference: " + juce::String (largestDifference));
    }
};

static MicroShifterTests microShifterTests;

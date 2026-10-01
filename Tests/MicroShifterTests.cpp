#include "MicroShifter.h"

#include <juce_core/juce_core.h>

#include <cmath>

class MicroShifterTests final : public juce::UnitTest
{
public:
    MicroShifterTests() : juce::UnitTest ("MicroShifter", "BigCity") {}

    void runTest() override
    {
        constexpr double sampleRate = 48000.0;
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
};

static MicroShifterTests microShifterTests;

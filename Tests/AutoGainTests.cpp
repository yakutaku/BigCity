#include "AutoGain.h"
#include "LoudnessMeasurement.h"
#include "SoundSpec.h"

#include <juce_core/juce_core.h>

class AutoGainTests final : public juce::UnitTest
{
public:
    AutoGainTests() : juce::UnitTest ("AutoGain", "BigCity") {}

    void runTest() override
    {
        testCurve();
        testLoudnessMatchesBypass();
    }

private:
    static constexpr double loudnessToleranceDb = 0.5;   // PRD §6 "Auto gain"

    void testCurve()
    {
        beginTest ("The curve is exactly 0 dB at 0, hits each measured point, interpolates in dB and clamps");

        expect (juce::exactlyEqual (AutoGain::gainDbFor (0.0f), 0.0f), "not exactly 0 dB at City Size 0");
        expect (juce::exactlyEqual ((float) (SoundSpec::autoGainDb.size() - 1) * SoundSpec::autoGainStepPercent,
                                    SoundSpec::citySizeMaxPercent - SoundSpec::citySizeMinPercent),
                "the auto gain points don't cover the whole City Size range");

        for (size_t point = 0; point < SoundSpec::autoGainDb.size(); ++point)
            expectWithinAbsoluteError (AutoGain::gainDbFor ((float) point * SoundSpec::autoGainStepPercent), SoundSpec::autoGainDb[point], 1.0e-6f);

        const auto midway = (SoundSpec::autoGainDb[4] + SoundSpec::autoGainDb[5]) / 2.0f;
        expectWithinAbsoluteError (AutoGain::gainDbFor (45.0f), midway, 1.0e-5f);

        expect (juce::exactlyEqual (AutoGain::gainDbFor (-10.0f), SoundSpec::autoGainDb.front()), "below 0 not clamped");
        expect (juce::exactlyEqual (AutoGain::gainDbFor (150.0f), SoundSpec::autoGainDb.back()), "above 100 not clamped");
    }

    void testLoudnessMatchesBypass()
    {
        beginTest ("Programme simulation noise at every 10 of City Size is within 0.5 dB of bypass (K-weighted, 48 kHz)");

        constexpr int stereo = 2, mono = 1;

        for (size_t point = 0; point < SoundSpec::autoGainDb.size(); ++point)
        {
            const auto citySize = (float) point * SoundSpec::autoGainStepPercent;
            const auto stereoDb = LoudnessMeasurement::differenceFromBypassDb (citySize, stereo);
            const auto monoDb = LoudnessMeasurement::differenceFromBypassDb (citySize, mono);

            logMessage ("  " + juce::String (citySize, 0) + ": stereo " + juce::String (stereoDb, 2) + " dB, mono " + juce::String (monoDb, 2) + " dB");
            expectWithinAbsoluteError (stereoDb, 0.0, loudnessToleranceDb, "stereo at City Size " + juce::String (citySize, 0));
            expectWithinAbsoluteError (monoDb, 0.0, loudnessToleranceDb, "mono at City Size " + juce::String (citySize, 0));
        }
    }
};

static AutoGainTests autoGainTests;

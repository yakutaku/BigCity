#include "WetDrive.h"

#include <juce_core/juce_core.h>

#include <cmath>

class WetDriveTests final : public juce::UnitTest
{
public:
    WetDriveTests() : juce::UnitTest ("WetDrive", "BigCity") {}

    void runTest() override
    {
        testQuietSignalsPassAtUnity();
        testPeaksAreRoundedOff();
        testDriveGlides();
    }

private:
    static constexpr float maxDriveDb = 12.0f;   // top of the tuning range

    void testQuietSignalsPassAtUnity()
    {
        beginTest ("Quiet signals pass at unity gain at every drive");

        constexpr float quietInput = 0.001f;   // -60 dBFS

        for (auto driveDb : { 0.0f, 3.0f, 6.0f, maxDriveDb })
        {
            const auto output = WetDrive::saturate (quietInput, juce::Decibels::decibelsToGain (driveDb));
            expectWithinAbsoluteError (juce::Decibels::gainToDecibels (output / quietInput), 0.0f, 0.001f);
        }
    }

    void testPeaksAreRoundedOff()
    {
        beginTest ("Peaks are rounded off more as drive rises, and never exceed 1 / drive gain");

        float previousOutput = 2.0f;

        for (auto driveDb : { 0.0f, 3.0f, 6.0f, maxDriveDb })
        {
            const auto driveGain = juce::Decibels::decibelsToGain (driveDb);
            const auto output = WetDrive::saturate (1.0f, driveGain);
            expect (output < previousOutput, "more drive should compress a full-scale peak more");
            expect (output <= 1.0f / driveGain, "output above the saturation ceiling");
            expect (juce::exactlyEqual (WetDrive::saturate (-1.0f, driveGain), -output), "saturation should be symmetric");
            previousOutput = output;
        }
    }

    void testDriveGlides()
    {
        beginTest ("A drive change glides over about 30 ms instead of jumping");

        constexpr double sampleRate = 48000.0;

        WetDrive drive;
        drive.setDriveDb (3.0f);
        drive.prepare (sampleRate);
        expectWithinAbsoluteError (juce::Decibels::gainToDecibels (drive.nextDriveGain()), 3.0f, 1.0e-4f);

        drive.setDriveDb (maxDriveDb);
        const auto afterOneSample = juce::Decibels::gainToDecibels (drive.nextDriveGain());
        expect (afterOneSample > 3.0f && afterOneSample < 3.1f, "drive jumped to " + juce::String (afterOneSample) + " dB");

        for (int i = 0; i < (int) (sampleRate * 0.030); ++i)
            drive.nextDriveGain();

        expectWithinAbsoluteError (juce::Decibels::gainToDecibels (drive.nextDriveGain()), maxDriveDb, 1.0e-4f);
    }
};

static WetDriveTests wetDriveTests;

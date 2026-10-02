#pragma once

#include "SoundSpec.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>

// Gentle tanh soft-saturation for the micro-shift's wet signal. Dividing by the drive gain
// level-compensates it: quiet signals pass at unity, and only peaks are rounded off.
class WetDrive
{
public:
    static float saturate (float input, float driveGain)
    {
        return std::tanh (driveGain * input) / driveGain;
    }

    // Glides to this drive. Set it before prepare to start there instead.
    void setDriveDb (float targetDb)
    {
        driveGain.setTargetValue (juce::Decibels::decibelsToGain (targetDb));
    }

    // Same glide time as City Size, so every control responds alike. Multiplicative, so the glide is even in dB.
    void prepare (double sampleRate)
    {
        driveGain.reset (sampleRate, SoundSpec::citySizeSmoothingSeconds);
    }

    // Call once per sample frame; every channel in that frame uses the same gain.
    float nextDriveGain()
    {
        return driveGain.getNextValue();
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> driveGain;
};

#pragma once

#include "SoundSpec.h"

#include <juce_audio_basics/juce_audio_basics.h>

// Fades between the processed and dry signals so bypass never clicks (PRD §6 "Bypass crossfade").
class BypassCrossfade
{
public:
    void prepare (double sampleRate, bool startBypassed)
    {
        dryProportion.reset (sampleRate, SoundSpec::bypassCrossfadeSeconds);
        dryProportion.setCurrentAndTargetValue (startBypassed ? 1.0f : 0.0f);
    }

    void setBypassed (bool bypassed)
    {
        dryProportion.setTargetValue (bypassed ? 1.0f : 0.0f);
    }

    float nextDryProportion()
    {
        return dryProportion.getNextValue();
    }

    // Weighted this way so proportions of exactly 0 and 1 return the processed or dry sample untouched.
    static float blend (float processed, float dry, float proportionOfDry)
    {
        return (1.0f - proportionOfDry) * processed + proportionOfDry * dry;
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dryProportion;
};

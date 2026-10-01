#include "CityMacro.h"
#include "SoundSpec.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <limits>

namespace
{
    constexpr float minusInfinityDb = -std::numeric_limits<float>::infinity();

    float interpolate (float from, float to, float proportion)
    {
        return from + (to - from) * proportion;
    }

    // "Off" has no dB value, so the lower half fades in linearly in amplitude (PRD "wet level curve").
    float wetLevelDbInLowerHalf (float proportion)
    {
        const auto gainAtMidpoint = juce::Decibels::decibelsToGain (SoundSpec::wetLevelDbAtMidpoint);
        return juce::Decibels::gainToDecibels (interpolate (0.0f, gainAtMidpoint, proportion), minusInfinityDb);
    }
}

CitySettings CityMacro::map (float citySizePercent)
{
    using namespace SoundSpec;

    const auto citySize = juce::jlimit (citySizeMinPercent, citySizeMaxPercent, citySizePercent);
    const auto inLowerHalf = citySize < curveMidpointPercent;
    const auto proportionOfHalf = inLowerHalf
        ? (citySize - citySizeMinPercent) / (curveMidpointPercent - citySizeMinPercent)
        : (citySize - curveMidpointPercent) / (citySizeMaxPercent - curveMidpointPercent);

    CitySettings settings;
    settings.airGainDb = airMaxGainDb * (citySize - citySizeMinPercent) / (citySizeMaxPercent - citySizeMinPercent);

    if (inLowerHalf)
    {
        settings.detuneCents = interpolate (detuneCentsAtMin, detuneCentsAtMidpoint, proportionOfHalf);
        settings.wetLevelDb = wetLevelDbInLowerHalf (proportionOfHalf);
    }
    else
    {
        settings.detuneCents = interpolate (detuneCentsAtMidpoint, detuneCentsAtMax, proportionOfHalf);
        settings.wetLevelDb = interpolate (wetLevelDbAtMidpoint, wetLevelDbAtMax, proportionOfHalf);
    }

    return settings;
}

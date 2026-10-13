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
    float wetLevelDbInLowerHalf (float proportion, float wetLevelDbAtMidpoint)
    {
        const auto gainAtMidpoint = juce::Decibels::decibelsToGain (wetLevelDbAtMidpoint);
        return juce::Decibels::gainToDecibels (interpolate (0.0f, gainAtMidpoint, proportion), minusInfinityDb);
    }

    float proportionOfRange (float citySize)
    {
        return (citySize - SoundSpec::citySizeMinPercent) / (SoundSpec::citySizeMaxPercent - SoundSpec::citySizeMinPercent);
    }

    // PRD §6 "Knob response": a parabola through 0, the bent midpoint and 100, so the bend has no
    // corner. It keeps rising as long as the bent midpoint is below 75.
    float bentCitySize (float citySize)
    {
        using namespace SoundSpec;

        const auto proportion = proportionOfRange (citySize);
        const auto bendAmount = 4.0f * (proportionOfRange (bentMidpointPercent) - 0.5f);
        const auto bentProportion = proportion + bendAmount * proportion * (1.0f - proportion);
        return citySizeMinPercent + bentProportion * (citySizeMaxPercent - citySizeMinPercent);
    }
}

CitySettings CityMacro::map (float citySizePercent, const SoundTuning& tuning)
{
    using namespace SoundSpec;

    const auto citySize = bentCitySize (juce::jlimit (citySizeMinPercent, citySizeMaxPercent, citySizePercent));
    const auto inLowerHalf = citySize < curveMidpointPercent;
    const auto proportionOfHalf = inLowerHalf
        ? (citySize - citySizeMinPercent) / (curveMidpointPercent - citySizeMinPercent)
        : (citySize - curveMidpointPercent) / (citySizeMaxPercent - curveMidpointPercent);

    // Each curve scales with its maximum so its shape holds. At the default tuning the scale is
    // exactly 1 and the offset exactly 0, so shipping builds get the PRD curves bit for bit.
    const auto detuneScale = tuning.detuneCentsAtMax / detuneCentsAtMax;
    const auto wetOffsetDb = tuning.wetLevelDbAtMax - wetLevelDbAtMax;

    CitySettings settings;
    settings.airGainDb = tuning.airMaxGainDb * proportionOfRange (citySize);
    settings.airCornerHz = tuning.airCornerHz;
    settings.airShelfQ = tuning.airShelfQ;

    if (inLowerHalf)
    {
        settings.detuneCents = interpolate (detuneCentsAtMin, detuneCentsAtMidpoint, proportionOfHalf) * detuneScale;
        settings.wetLevelDb = wetLevelDbInLowerHalf (proportionOfHalf, wetLevelDbAtMidpoint + wetOffsetDb);
    }
    else
    {
        settings.detuneCents = interpolate (detuneCentsAtMidpoint, detuneCentsAtMax, proportionOfHalf) * detuneScale;
        settings.wetLevelDb = interpolate (wetLevelDbAtMidpoint + wetOffsetDb, wetLevelDbAtMax + wetOffsetDb, proportionOfHalf);
    }

    return settings;
}

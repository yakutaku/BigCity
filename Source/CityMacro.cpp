#include "CityMacro.h"
#include "SoundSpec.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
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

    float bendAmount()
    {
        return 4.0f * (proportionOfRange (SoundSpec::bentMidpointPercent) - 0.5f);
    }

    // PRD §6 "Knob response": a parabola through 0, the bent midpoint and 100, so the bend has no
    // corner. It keeps rising as long as the bent midpoint is below 75.
    float bentCitySize (float citySize)
    {
        using namespace SoundSpec;

        const auto proportion = proportionOfRange (citySize);
        const auto bentProportion = proportion + bendAmount() * proportion * (1.0f - proportion);
        return citySizeMinPercent + bentProportion * (citySizeMaxPercent - citySizeMinPercent);
    }

    // Voicing 1 is 1.0's mapping, kept exactly as it shipped so old sessions sound the same bit for bit.
    CitySettings mapVoicing1 (float citySizePercent, const SoundTuning& tuning)
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

    // How fast each element is moving where voicing 1 reaches 100, per half of voicing 2's knob (which
    // spans voicing 1's whole range): the bend's slope there times the curve's slope in its upper half.
    struct UpperHalfRates
    {
        float airGainDb, detuneCents, wetLevelDb;
    };

    UpperHalfRates upperHalfRates (const SoundTuning& tuning)
    {
        using namespace SoundSpec;

        const auto bendSlopeAtMax = 1.0f - bendAmount();
        const auto upperCurveWidth = proportionOfRange (citySizeMaxPercent) - proportionOfRange (curveMidpointPercent);
        const auto detuneScale = tuning.detuneCentsAtMax / detuneCentsAtMax;

        return { bendSlopeAtMax * tuning.airMaxGainDb,
                 bendSlopeAtMax * (detuneCentsAtMax - detuneCentsAtMidpoint) * detuneScale / upperCurveWidth,
                 bendSlopeAtMax * (wetLevelDbAtMax - wetLevelDbAtMidpoint) / upperCurveWidth };
    }

    // Carries an element from its value at 50 on to its top at 100 (proportion 0 to 1). It leaves at the
    // rate voicing 1 arrived, so there's no corner at 50, and it never turns down: a top too close to
    // reach that way is eased into early and then held.
    float continueToTop (float start, float startRate, float top, float proportion)
    {
        const auto rise = top - start;

        if (rise <= 0.0f)
            return start;

        if (rise >= startRate / 2.0f)
            return start + startRate * proportion + (rise - startRate) * proportion * proportion;

        const auto proportionAtTop = 2.0f * rise / startRate;
        const auto easedProportion = std::min (proportion, proportionAtTop);
        return start + startRate * easedProportion - startRate * startRate / (4.0f * rise) * easedProportion * easedProportion;
    }

    // PRD §6 "Voicing 2"
    CitySettings mapVoicing2 (float citySizePercent, const SoundTuning& tuning)
    {
        using namespace SoundSpec;

        const auto citySize = juce::jlimit (citySizeMinPercent, citySizeMaxPercent, citySizePercent);

        if (citySize <= curveMidpointPercent)
        {
            // Exactly 2, so 50 lands on voicing 1's 100 bit for bit.
            const auto stretch = (citySizeMaxPercent - citySizeMinPercent) / (curveMidpointPercent - citySizeMinPercent);
            return mapVoicing1 (citySizeMinPercent + (citySize - citySizeMinPercent) * stretch, tuning);
        }

        const auto proportion = (citySize - curveMidpointPercent) / (citySizeMaxPercent - curveMidpointPercent);
        const auto rates = upperHalfRates (tuning);

        auto settings = mapVoicing1 (citySizeMaxPercent, tuning);
        settings.airGainDb = continueToTop (settings.airGainDb, rates.airGainDb, voicing2AirGainDbAtMax, proportion);
        settings.detuneCents = continueToTop (settings.detuneCents, rates.detuneCents, voicing2DetuneCentsAtMax, proportion);
        settings.wetLevelDb = continueToTop (settings.wetLevelDb, rates.wetLevelDb, voicing2WetLevelDbAtMax, proportion);
        return settings;
    }
}

CitySettings CityMacro::map (float citySizePercent, Voicing voicing, const SoundTuning& tuning)
{
    return voicing == Voicing::original ? mapVoicing1 (citySizePercent, tuning)
                                        : mapVoicing2 (citySizePercent, tuning);
}

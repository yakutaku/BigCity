#include "AutoGain.h"
#include "SoundSpec.h"

#include <juce_core/juce_core.h>

#include <cmath>

namespace
{
    const std::array<float, 11>& curveFor (Voicing voicing)
    {
        switch (voicing)
        {
            case Voicing::a: return SoundSpec::autoGainDbVoicingA;
            case Voicing::b: return SoundSpec::autoGainDbVoicingB;
            case Voicing::c: return SoundSpec::autoGainDbVoicingC;
            case Voicing::current: break;
        }

        return SoundSpec::autoGainDb;
    }
}

float AutoGain::gainDbFor (float citySizePercent, Voicing voicing)
{
    using namespace SoundSpec;

    const auto& curve = curveFor (voicing);
    const auto position = (juce::jlimit (citySizeMinPercent, citySizeMaxPercent, citySizePercent) - citySizeMinPercent) / autoGainStepPercent;
    const auto lastIndex = (int) curve.size() - 1;
    const auto index = std::min ((int) std::floor (position), lastIndex - 1);
    const auto proportion = position - (float) index;

    const auto from = curve[(size_t) index];
    const auto to = curve[(size_t) index + 1];
    return from + (to - from) * proportion;
}

#include "AutoGain.h"
#include "SoundSpec.h"

#include <juce_core/juce_core.h>

#include <cmath>

float AutoGain::gainDbFor (float citySizePercent)
{
    using namespace SoundSpec;

    const auto position = (juce::jlimit (citySizeMinPercent, citySizeMaxPercent, citySizePercent) - citySizeMinPercent) / autoGainStepPercent;
    const auto lastIndex = (int) autoGainDb.size() - 1;
    const auto index = std::min ((int) std::floor (position), lastIndex - 1);
    const auto proportion = position - (float) index;

    const auto from = autoGainDb[(size_t) index];
    const auto to = autoGainDb[(size_t) index + 1];
    return from + (to - from) * proportion;
}

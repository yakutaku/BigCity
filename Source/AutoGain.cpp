#include "AutoGain.h"
#include "SoundSpec.h"

#include <juce_core/juce_core.h>

#include <cmath>

float AutoGain::gainDbFor (float citySizePercent)
{
    using namespace SoundSpec;

    static_assert (autoGainDb.front() == 0.0f, "City Size 0 must null against bypass");
    static_assert ((autoGainDb.size() - 1) * autoGainStepPercent == citySizeMaxPercent - citySizeMinPercent,
                   "one auto gain point per step across the whole City Size range");

    const auto position = (juce::jlimit (citySizeMinPercent, citySizeMaxPercent, citySizePercent) - citySizeMinPercent) / autoGainStepPercent;
    const auto lastIndex = (int) autoGainDb.size() - 1;
    const auto index = std::min ((int) std::floor (position), lastIndex - 1);
    const auto proportion = position - (float) index;

    const auto from = autoGainDb[(size_t) index];
    const auto to = autoGainDb[(size_t) index + 1];
    return from + (to - from) * proportion;
}

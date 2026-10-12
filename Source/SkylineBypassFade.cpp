#include "SkylineBypassFade.h"
#include "UiSpec.h"

#include <juce_core/juce_core.h>

namespace
{
    // Cubic ease-in-out: a gentle start and a soft landing, the in-out partner of the knob's ease-out (D-25).
    float easeInOut (float progress)
    {
        if (progress < 0.5f)
            return 4.0f * progress * progress * progress;

        const auto fromEnd = 2.0f - 2.0f * progress;
        return 1.0f - fromEnd * fromEnd * fromEnd / 2.0f;
    }
}

SkylineBypassFade::SkylineBypassFade (bool initiallyBypassed)
    : progress (initiallyBypassed ? 1.0f : 0.0f),
      targetProgress (progress),
      fadeStartProgress (progress)
{
}

float SkylineBypassFade::advance (bool bypassed, double nowSeconds)
{
    const auto target = bypassed ? 1.0f : 0.0f;

    if (! juce::exactlyEqual (target, targetProgress))
    {
        targetProgress = target;
        fadeStartProgress = progress;
        fadeStartSeconds = nowSeconds;
    }

    const auto travelled = (float) ((nowSeconds - fadeStartSeconds) * 1000.0 / UiSpec::Skyline::bypassFadeMs);

    progress = target > fadeStartProgress ? std::min (target, fadeStartProgress + travelled)
                                          : std::max (target, fadeStartProgress - travelled);

    return easeInOut (progress);
}

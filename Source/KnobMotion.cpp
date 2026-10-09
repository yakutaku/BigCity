#include "KnobMotion.h"
#include "UiSpec.h"

namespace
{
    // Cubic ease-out: most of the way there at once, then a soft landing.
    float easeOut (float progress)
    {
        const auto remaining = 1.0f - progress;
        return 1.0f - remaining * remaining * remaining;
    }

    bool startsGlide (float percentMovedThisFrame, KnobMotion::Change change)
    {
        switch (change)
        {
            case KnobMotion::Change::follow: return false;
            case KnobMotion::Change::glide:  return true;
            case KnobMotion::Change::judgeBySize: break;
        }

        return std::abs (percentMovedThisFrame) > UiSpec::Knob::glideJumpPercent;
    }
}

KnobMotion::KnobMotion (float initialPercent)
    : shownPercent (initialPercent),
      targetPercent (initialPercent)
{
}

bool KnobMotion::advance (float newTargetPercent, double nowSeconds, Change change)
{
    const auto previouslyShown = shownPercent;

    if (! juce::exactlyEqual (newTargetPercent, targetPercent))
    {
        // A new glide starts from wherever the knob is now, so retargeting mid-glide never jumps.
        if (startsGlide (newTargetPercent - targetPercent, change))
        {
            glideStartPercent = shownPercent;
            glideStartSeconds = nowSeconds;
            gliding = true;
        }

        // A small change mid-glide just moves the landing point; snapping to it would jump.
        targetPercent = newTargetPercent;
    }

    if (gliding)
        moveAlongGlide (nowSeconds);
    else
        shownPercent = targetPercent;

    return ! juce::exactlyEqual (shownPercent, previouslyShown);
}

void KnobMotion::moveAlongGlide (double nowSeconds)
{
    const auto progress = (float) ((nowSeconds - glideStartSeconds) * 1000.0 / UiSpec::Knob::glideMs);

    if (progress >= 1.0f)
    {
        shownPercent = targetPercent;
        gliding = false;
        return;
    }

    shownPercent = glideStartPercent + (targetPercent - glideStartPercent) * easeOut (std::max (0.0f, progress));
}

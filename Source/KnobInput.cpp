#include "KnobInput.h"
#include "SoundSpec.h"
#include "UiSpec.h"

namespace
{
    float clampToRange (float citySizePercent)
    {
        return juce::jlimit (SoundSpec::citySizeMinPercent, SoundSpec::citySizeMaxPercent, citySizePercent);
    }

    // Follows juce::Slider, so the knob scrolls the same way as other JUCE plugins: Shift turns a mouse
    // wheel's vertical scroll into horizontal on macOS, and a reversed (natural) scroll is flipped back.
    float wheelAmount (const juce::MouseWheelDetails& wheel)
    {
        const auto amount = std::abs (wheel.deltaX) > std::abs (wheel.deltaY) ? -wheel.deltaX : wheel.deltaY;
        return wheel.isReversed ? -amount : amount;
    }

    // How far a burst's total movement reaches past the dead zone, keeping its direction.
    float beyondDeadZone (float pixelsUp)
    {
        const auto beyond = std::max (0.0f, std::abs (pixelsUp) - UiSpec::Knob::trackpadDeadZonePixels);
        return pixelsUp < 0.0f ? -beyond : beyond;
    }
}

float KnobInput::angleRadiansFor (float citySizePercent)
{
    using namespace UiSpec::Knob;
    return juce::degreesToRadians (angleAtZeroDegrees + degreesPerPercent * clampToRange (citySizePercent));
}

float KnobInput::valueAfterDrag (float citySizePercent, float pixelsUp, bool fine)
{
    using namespace UiSpec::Knob;
    constexpr float fullRangePercent = SoundSpec::citySizeMaxPercent - SoundSpec::citySizeMinPercent;
    const auto percentPerPixel = fine ? fineDragPercentPerPixel : fullRangePercent / dragPixelsForFullRange;
    return clampToRange (citySizePercent + pixelsUp * percentPerPixel);
}

float KnobInput::valueAfterNotch (float citySizePercent, const juce::MouseWheelDetails& wheel, bool fine)
{
    using namespace UiSpec::Knob;
    const auto amount = wheelAmount (wheel);

    if (juce::exactlyEqual (amount, 0.0f))
        return citySizePercent;

    const auto notchPercent = fine ? fineWheelNotchPercent : wheelNotchPercent;
    return clampToRange (citySizePercent + (amount > 0.0f ? notchPercent : -notchPercent));
}

float KnobInput::trackpadPixelsUp (const juce::MouseWheelDetails& wheel)
{
    return wheelAmount (wheel) * UiSpec::Knob::trackpadPixelsPerWheelUnit;
}

float KnobInput::TrackpadDeadZone::pixelsToApply (float pixelsUp)
{
    const auto beyondBefore = beyondDeadZone (pixelsInBurst);
    pixelsInBurst += pixelsUp;
    return beyondDeadZone (pixelsInBurst) - beyondBefore;
}

void KnobInput::TrackpadDeadZone::reset()
{
    pixelsInBurst = 0.0f;
}

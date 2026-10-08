#include "CityKnob.h"
#include "KnobInput.h"
#include "Tokens.h"
#include "UiSpec.h"

#include <BinaryData.h>

CityKnob::CityKnob (juce::AudioParameterFloat& citySizeParameter)
    : citySize (citySizeParameter),
      base (juce::ImageCache::getFromMemory (BinaryData::knob_base_png, BinaryData::knob_base_pngSize)),
      rotor (juce::ImageCache::getFromMemory (BinaryData::knob_rotor_png, BinaryData::knob_rotor_pngSize)),
      displayedPercent (citySizeParameter.get())
{
    setBounds (UiSpec::Knob::imageBounds);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setRepaintsOnMouseActivity (true);
}

CityKnob::~CityKnob()
{
    // A host left with an open gesture keeps the parameter armed for automation recording.
    endDragGesture();
    endScrollBurst();
}

void CityKnob::setDisplayedPercent (float citySizePercent)
{
    if (juce::exactlyEqual (citySizePercent, displayedPercent))
        return;

    displayedPercent = citySizePercent;
    repaint();
}

void CityKnob::paint (juce::Graphics& g)
{
    g.drawImage (base, getLocalBounds().toFloat());
    drawRotor (g);
    drawOverlay (g);
}

bool CityKnob::hitTest (int x, int y)
{
    // Only the cap takes the mouse: not the ticks, the arc or the shadow around it.
    return juce::Point<float> ((float) x, (float) y).getDistanceFrom (UiSpec::Knob::centre) <= UiSpec::Knob::capRadius;
}

void CityKnob::mouseDown (const juce::MouseEvent& e)
{
    endScrollBurst();

    if (e.mods.isAltDown())
    {
        resetToDefault();
        return;
    }

    citySize.beginChangeGesture();
    dragGestureActive = true;
    gesturePercent = citySize.get();
    lastDragY = e.position.y;
}

void CityKnob::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragGestureActive)
        return;

    // Shift is read on every move, so pressing or releasing it mid-drag changes speed without a jump.
    gesturePercent = KnobInput::valueAfterDrag (gesturePercent, lastDragY - e.position.y, e.mods.isShiftDown());
    lastDragY = e.position.y;
    sendToHost (gesturePercent);
}

void CityKnob::mouseUp (const juce::MouseEvent&)
{
    endDragGesture();
}

void CityKnob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    // macOS sometimes delivers the same wheel event twice; juce::Slider ignores repeats the same way.
    if (dragGestureActive || e.eventTime == lastWheelEventTime)
        return;

    lastWheelEventTime = e.eventTime;

    // Every event extends the burst, including trackpad movement still inside the dead zone.
    startTimer (UiSpec::Knob::scrollGestureEndMs);

    const auto fine = e.mods.isShiftDown();

    if (wheel.isSmooth)
    {
        const auto pixelsUp = trackpadDeadZone.pixelsToApply (KnobInput::trackpadPixelsUp (wheel));

        // A brush that stays inside the dead zone neither moves the knob nor opens a gesture.
        if (juce::exactlyEqual (pixelsUp, 0.0f))
            return;

        beginScrollGesture();
        gesturePercent = KnobInput::valueAfterDrag (gesturePercent, pixelsUp, fine);
    }
    else
    {
        beginScrollGesture();
        gesturePercent = KnobInput::valueAfterNotch (gesturePercent, wheel, fine);
    }

    sendToHost (gesturePercent);
}

void CityKnob::timerCallback()
{
    endScrollBurst();
}

void CityKnob::resetToDefault()
{
    // AudioParameterFloat hides the normalised accessors; they're public on the host-facing base.
    const juce::AudioProcessorParameter& hostParameter = citySize;

    citySize.beginChangeGesture();
    citySize.setValueNotifyingHost (hostParameter.getDefaultValue());
    citySize.endChangeGesture();
}

void CityKnob::endDragGesture()
{
    if (! dragGestureActive)
        return;

    citySize.endChangeGesture();
    dragGestureActive = false;
}

void CityKnob::beginScrollGesture()
{
    if (scrollGestureActive)
        return;

    citySize.beginChangeGesture();
    scrollGestureActive = true;
    gesturePercent = citySize.get();
}

void CityKnob::endScrollBurst()
{
    stopTimer();
    trackpadDeadZone.reset();

    if (! scrollGestureActive)
        return;

    citySize.endChangeGesture();
    scrollGestureActive = false;
}

void CityKnob::sendToHost (float citySizePercent)
{
    const juce::AudioProcessorParameter& hostParameter = citySize;
    const auto normalised = citySize.convertTo0to1 (citySizePercent);

    // Only real steps go to the host, so automation isn't flooded with repeats of the same value.
    if (! juce::exactlyEqual (normalised, hostParameter.getValue()))
        citySize.setValueNotifyingHost (normalised);
}

void CityKnob::drawRotor (juce::Graphics& g) const
{
    const auto pointsPerImagePixel = (float) getWidth() / (float) rotor.getWidth();
    const auto pivot = UiSpec::Knob::centre;

    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageTransformed (rotor, juce::AffineTransform::scale (pointsPerImagePixel)
                                       .rotated (KnobInput::angleRadiansFor (displayedPercent), pivot.x, pivot.y));
}

void CityKnob::drawOverlay (juce::Graphics& g) const
{
    juce::Colour overlay;

    if (isMouseButtonDown())
        overlay = juce::Colour (Tokens::Colours::inkPrimary).withAlpha (Tokens::Overlay::pressAlpha);
    else if (isMouseOver())
        overlay = juce::Colour (Tokens::Colours::hoverOverlayBase).withAlpha (Tokens::Overlay::hoverAlpha);
    else
        return;

    const auto capDiameter = UiSpec::Knob::capRadius * 2.0f;
    g.setColour (overlay);
    g.fillEllipse (juce::Rectangle<float> (capDiameter, capDiameter).withCentre (UiSpec::Knob::centre));
}

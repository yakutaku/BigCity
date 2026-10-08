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
    endScrollGesture();
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
    // The images overhang the knob to fit its shadow; only the knob itself takes the mouse.
    return getLocalBounds().reduced (UiSpec::Knob::hitAreaInset).contains (x, y);
}

void CityKnob::mouseDown (const juce::MouseEvent& e)
{
    endScrollGesture();

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

    if (! scrollGestureActive)
    {
        citySize.beginChangeGesture();
        scrollGestureActive = true;
        gesturePercent = citySize.get();
    }

    gesturePercent = KnobInput::valueAfterWheel (gesturePercent, wheel, e.mods.isShiftDown());
    sendToHost (gesturePercent);
    startTimer (UiSpec::Knob::scrollGestureEndMs);
}

void CityKnob::timerCallback()
{
    endScrollGesture();
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

void CityKnob::endScrollGesture()
{
    stopTimer();

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
    const auto pivot = UiSpec::Knob::rotorPivot;

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

    const auto inset = (float) UiSpec::Knob::hitAreaInset;
    g.setColour (overlay);
    g.fillEllipse (UiSpec::Knob::overlayEllipse.translated (inset, inset));
}

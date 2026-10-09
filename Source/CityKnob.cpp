#include "CityKnob.h"
#include "InteractiveOverlay.h"
#include "KnobInput.h"
#include "KnobPointer.h"
#include "UiSpec.h"

#include <BinaryData.h>

CityKnob::CityKnob (juce::AudioParameterFloat& citySizeParameter)
    : citySize (citySizeParameter),
      staticLayers (composeStaticLayers()),
      rotor (juce::ImageCache::getFromMemory (BinaryData::knob_rotor_png, BinaryData::knob_rotor_pngSize)),
      motion (citySizeParameter.get()),
      displayRefresh (this, [this] (double timestampSeconds) { advanceFrame (timestampSeconds); })
{
    // staticLayers includes the panel, so nothing behind the knob needs redrawing on each frame.
    setOpaque (true);
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

void CityKnob::advanceFrame (double timestampSeconds)
{
    const auto change = std::exchange (nextChange, KnobMotion::Change::judgeBySize);

    if (motion.advance (citySize.get(), timestampSeconds, change))
        repaint();
}

void CityKnob::paint (juce::Graphics& g)
{
    const auto angleRadians = KnobInput::angleRadiansFor (motion.getShownPercent());

    g.drawImage (staticLayers, getLocalBounds().toFloat());
    drawRotor (g, angleRadians);
    KnobPointer::draw (g, angleRadians);
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
    sendToHost (gesturePercent, KnobMotion::Change::follow);
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
        sendToHost (gesturePercent, KnobMotion::Change::follow);
    }
    else
    {
        beginScrollGesture();
        gesturePercent = KnobInput::valueAfterNotch (gesturePercent, wheel, fine);
        sendToHost (gesturePercent, KnobMotion::Change::glide);
    }
}

void CityKnob::timerCallback()
{
    endScrollBurst();
}

void CityKnob::resetToDefault()
{
    // AudioParameterFloat hides the normalised accessors; they're public on the host-facing base.
    const juce::AudioProcessorParameter& hostParameter = citySize;

    nextChange = KnobMotion::Change::glide;
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

void CityKnob::sendToHost (float citySizePercent, KnobMotion::Change change)
{
    const juce::AudioProcessorParameter& hostParameter = citySize;
    const auto normalised = citySize.convertTo0to1 (citySizePercent);

    // Only real steps go to the host, so automation isn't flooded with repeats of the same value.
    if (juce::exactlyEqual (normalised, hostParameter.getValue()))
        return;

    nextChange = change;
    citySize.setValueNotifyingHost (normalised);
}

juce::Image CityKnob::composeStaticLayers()
{
    const auto panel = juce::ImageCache::getFromMemory (BinaryData::panel_png, BinaryData::panel_pngSize);
    const auto base = juce::ImageCache::getFromMemory (BinaryData::knob_base_png, BinaryData::knob_base_pngSize);

    // Both images are @2x, so the panel's pixels line up with knob_base's and nothing is resampled.
    const auto pixelsPerPoint = base.getWidth() / UiSpec::Knob::imageBounds.getWidth();
    const auto knobOriginInPanel = UiSpec::Knob::imageBounds.getPosition() * pixelsPerPoint;

    juce::Image layers (juce::Image::ARGB, base.getWidth(), base.getHeight(), true);
    juce::Graphics g (layers);
    g.drawImageAt (panel, -knobOriginInPanel.x, -knobOriginInPanel.y);
    g.drawImageAt (base, 0, 0);
    return layers;
}

void CityKnob::drawRotor (juce::Graphics& g, float angleRadians) const
{
    const auto pointsPerImagePixel = (float) getWidth() / (float) rotor.getWidth();
    const auto pivot = UiSpec::Knob::centre;

    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageTransformed (rotor, juce::AffineTransform::scale (pointsPerImagePixel)
                                       .rotated (angleRadians, pivot.x, pivot.y));
}

void CityKnob::drawOverlay (juce::Graphics& g) const
{
    const auto overlay = InteractiveOverlay::colourFor (*this);

    if (! overlay.has_value())
        return;

    const auto capDiameter = UiSpec::Knob::capRadius * 2.0f;
    g.setColour (*overlay);
    g.fillEllipse (juce::Rectangle<float> (capDiameter, capDiameter).withCentre (UiSpec::Knob::centre));
}

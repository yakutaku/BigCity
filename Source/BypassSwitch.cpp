#include "BypassSwitch.h"
#include "InteractiveOverlay.h"
#include "UiSpec.h"

#include <BinaryData.h>

namespace
{
    bool isOn (const juce::AudioProcessorParameter& parameter)
    {
        return parameter.getValue() >= 0.5f;
    }
}

BypassSwitch::BypassSwitch (juce::AudioProcessorParameter& bypassParameter)
    : bypass (bypassParameter),
      fxOnImage (juce::ImageCache::getFromMemory (BinaryData::switch_fx_on_png, BinaryData::switch_fx_on_pngSize)),
      bypassedImage (juce::ImageCache::getFromMemory (BinaryData::switch_bypassed_png, BinaryData::switch_bypassed_pngSize)),
      displayedBypassed (isOn (bypassParameter))
{
    setBounds (UiSpec::BypassSwitch::imageBounds);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setRepaintsOnMouseActivity (true);
}

void BypassSwitch::setDisplayedBypassed (bool bypassed)
{
    if (bypassed == displayedBypassed)
        return;

    displayedBypassed = bypassed;
    repaint();
}

void BypassSwitch::paint (juce::Graphics& g)
{
    g.drawImage (displayedBypassed ? bypassedImage : fxOnImage, getLocalBounds().toFloat());
    drawOverlay (g);
}

bool BypassSwitch::hitTest (int x, int)
{
    // The image is wider than the switch only so the LED glow fits; the glow isn't part of the control.
    return x < UiSpec::BypassSwitch::hitWidth;
}

void BypassSwitch::mouseUp (const juce::MouseEvent& e)
{
    // Like a button: releasing outside the switch cancels the press.
    const auto release = e.getPosition();

    if (! getLocalBounds().contains (release) || ! hitTest (release.x, release.y))
        return;

    const auto bypassed = ! isOn (bypass);

    bypass.beginChangeGesture();
    bypass.setValueNotifyingHost (bypassed ? 1.0f : 0.0f);
    bypass.endChangeGesture();

    // Shown at once rather than on the editor's next poll, so the click feels immediate.
    setDisplayedBypassed (bypassed);
}

void BypassSwitch::drawOverlay (juce::Graphics& g) const
{
    const auto overlay = InteractiveOverlay::colourFor (*this);

    if (! overlay.has_value())
        return;

    g.setColour (*overlay);
    g.fillRoundedRectangle (UiSpec::BypassSwitch::trackOverlay, UiSpec::BypassSwitch::trackCornerRadius);
}

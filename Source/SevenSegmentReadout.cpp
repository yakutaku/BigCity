#include "SevenSegmentReadout.h"
#include "Tokens.h"
#include "UiSpec.h"

SevenSegmentReadout::SevenSegmentReadout (juce::AudioParameterFloat& citySizeParameter)
    : citySize (citySizeParameter),
      displayedDigits (SevenSegment::digitsFor (citySizeParameter.get())),
      litSegments (SevenSegment::litSegmentsFor (displayedDigits))
{
    setBounds (UiSpec::Readout::hitArea);
    setWantsKeyboardFocus (true);
}

void SevenSegmentReadout::setDisplayedPercent (float citySizePercent)
{
    if (! isEditing())
        showDigits (SevenSegment::digitsFor (citySizePercent));
}

void SevenSegmentReadout::beginEditing()
{
    if (isEditing())
        return;

    typedValue.emplace (citySize.get());
    showDigits (typedValue->digits());
    grabKeyboardFocus();
    repaint();
}

void SevenSegmentReadout::paint (juce::Graphics& g)
{
    if (isEditing())
        drawEditingFrame (g);

    const auto lcdOffset = UiSpec::Readout::lcdBounds.getPosition() - getPosition().toFloat();

    g.setColour (juce::Colour (Tokens::Colours::inkPrimary));
    g.fillPath (litSegments, juce::AffineTransform::translation (lcdOffset));
}

void SevenSegmentReadout::mouseDoubleClick (const juce::MouseEvent&)
{
    beginEditing();
}

bool SevenSegmentReadout::keyPressed (const juce::KeyPress& key)
{
    if (! isEditing())
        return false;

    if (key.getKeyCode() == juce::KeyPress::returnKey)
    {
        commitEditing();
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        endEditing();
        return true;
    }

    // Cmd shortcuts, such as saving the set, still reach the host while typing.
    if (key.getModifiers().isCommandDown())
        return false;

    if (key.getKeyCode() == juce::KeyPress::backspaceKey)
        typedValue->deleteBackwards();
    else
        typedValue->type (key.getTextCharacter());

    showDigits (typedValue->digits());
    repaint();

    // Every other plain key is swallowed, so typing a wrong key never triggers a host shortcut.
    return true;
}

void SevenSegmentReadout::focusLost (FocusChangeType)
{
    // Clicking anywhere else, in the plugin or the host, moves focus away; that commits, like Enter.
    if (isEditing())
        commitEditing();
}

void SevenSegmentReadout::commitEditing()
{
    const auto committedPercent = ReadoutEntry::parseCitySize (typedValue->getText());

    if (committedPercent.has_value())
        sendToHost (*committedPercent);

    endEditing();
}

void SevenSegmentReadout::endEditing()
{
    typedValue.reset();
    showDigits (SevenSegment::digitsFor (citySize.get()));
    repaint();
}

void SevenSegmentReadout::showDigits (const SevenSegment::Digits& digits)
{
    if (digits == displayedDigits)
        return;

    displayedDigits = digits;
    litSegments = SevenSegment::litSegmentsFor (digits);
    repaint();
}

void SevenSegmentReadout::sendToHost (float citySizePercent)
{
    const juce::AudioProcessorParameter& hostParameter = citySize;
    const auto normalised = citySize.convertTo0to1 (citySizePercent);

    // An unchanged value isn't an edit, so it doesn't write an automation point.
    if (juce::exactlyEqual (normalised, hostParameter.getValue()))
        return;

    citySize.beginChangeGesture();
    citySize.setValueNotifyingHost (normalised);
    citySize.endChangeGesture();
}

void SevenSegmentReadout::drawEditingFrame (juce::Graphics& g) const
{
    using namespace UiSpec::Readout;

    if (typedValue->isSelected())
    {
        g.setColour (juce::Colour (Tokens::Colours::pastelSky));
        g.fillRect (selection);
    }

    // Centred half a ring outside the LCD's edge, so the whole 2 pt ring sits outside it.
    const auto halfRing = focusRingThickness / 2.0f;
    const auto lcd = lcdBounds - getPosition().toFloat();

    g.setColour (juce::Colour (Tokens::Colours::accentTeal));
    g.drawRoundedRectangle (lcd.expanded (halfRing), lcdCornerRadius + halfRing, focusRingThickness);
}

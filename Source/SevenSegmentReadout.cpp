#include "SevenSegmentReadout.h"
#include "Tokens.h"
#include "UiSpec.h"

SevenSegmentReadout::SevenSegmentReadout (float initialCitySizePercent)
    : displayedDigits (SevenSegment::digitsFor (initialCitySizePercent)),
      litSegments (SevenSegment::litSegmentsFor (displayedDigits))
{
    setBounds (UiSpec::Readout::hitArea);
}

void SevenSegmentReadout::setDisplayedPercent (float citySizePercent)
{
    const auto digits = SevenSegment::digitsFor (citySizePercent);

    if (digits == displayedDigits)
        return;

    displayedDigits = digits;
    litSegments = SevenSegment::litSegmentsFor (digits);
    repaint();
}

void SevenSegmentReadout::paint (juce::Graphics& g)
{
    const auto lcdOffset = UiSpec::Readout::lcdOrigin - getPosition().toFloat();

    g.setColour (juce::Colour (Tokens::Colours::inkPrimary));
    g.fillPath (litSegments, juce::AffineTransform::translation (lcdOffset));
}

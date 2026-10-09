#include "PluginEditor.h"
#include "UiSpec.h"

#include <BinaryData.h>

BigCityAudioProcessorEditor::BigCityAudioProcessorEditor (BigCityAudioProcessor& bigCityProcessor)
    : AudioProcessorEditor (bigCityProcessor),
      citySize (bigCityProcessor.getCitySizeParameter()),
      bypass (*bigCityProcessor.getBypassParameter()),
      panel (juce::ImageCache::getFromMemory (BinaryData::panel_png, BinaryData::panel_pngSize)),
      knob (citySize),
      readout (citySize.get()),
      skyline (knob.getShownPercent(), isBypassed())
{
    // The panel covers every pixel, so nothing behind the editor needs drawing.
    setOpaque (true);
    setResizable (false, false);
    setSize (UiSpec::windowWidth, UiSpec::windowHeight);

    addAndMakeVisible (knob);
    addAndMakeVisible (readout);
    addAndMakeVisible (skyline);
    startTimerHz (UiSpec::repaintRateHz);
}

void BigCityAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImage (panel, UiSpec::panelBounds);
}

void BigCityAudioProcessorEditor::timerCallback()
{
    readout.setDisplayedPercent (citySize.get());

    // The skyline follows the knob's angle on screen, so the two move as one when the knob glides.
    skyline.setState (knob.getShownPercent(), isBypassed());
}

bool BigCityAudioProcessorEditor::isBypassed() const
{
    return bypass.getValue() >= 0.5f;
}

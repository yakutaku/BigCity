#include "PluginEditor.h"
#include "UiSpec.h"

#include <BinaryData.h>

BigCityAudioProcessorEditor::BigCityAudioProcessorEditor (BigCityAudioProcessor& bigCityProcessor)
    : AudioProcessorEditor (bigCityProcessor),
      citySize (bigCityProcessor.getCitySizeParameter()),
      panel (juce::ImageCache::getFromMemory (BinaryData::panel_png, BinaryData::panel_pngSize)),
      knob (citySize),
      readout (citySize.get())
{
    // The panel covers every pixel, so nothing behind the editor needs drawing.
    setOpaque (true);
    setResizable (false, false);
    setSize (UiSpec::windowWidth, UiSpec::windowHeight);

    addAndMakeVisible (knob);
    addAndMakeVisible (readout);
    startTimerHz (UiSpec::repaintRateHz);
}

void BigCityAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImage (panel, UiSpec::panelBounds);
}

void BigCityAudioProcessorEditor::timerCallback()
{
    readout.setDisplayedPercent (citySize.get());
}

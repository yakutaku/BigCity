#include "PluginEditor.h"
#include "UiSpec.h"

#include <BinaryData.h>

BigCityAudioProcessorEditor::BigCityAudioProcessorEditor (BigCityAudioProcessor& bigCityProcessor)
    : AudioProcessorEditor (bigCityProcessor),
      panel (juce::ImageCache::getFromMemory (BinaryData::panel_png, BinaryData::panel_pngSize))
{
    // The panel covers every pixel, so nothing behind the editor needs drawing.
    setOpaque (true);
    setResizable (false, false);
    setSize (UiSpec::windowWidth, UiSpec::windowHeight);
}

void BigCityAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImage (panel, UiSpec::panelBounds);
}

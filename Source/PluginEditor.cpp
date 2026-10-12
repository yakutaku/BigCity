#include "PluginEditor.h"
#include "UiSpec.h"

#include <BinaryData.h>

BigCityAudioProcessorEditor::BigCityAudioProcessorEditor (BigCityAudioProcessor& bigCityProcessor)
    : AudioProcessorEditor (bigCityProcessor),
      citySize (bigCityProcessor.getCitySizeParameter()),
      bypass (*bigCityProcessor.getBypassParameter()),
      panel (juce::ImageCache::getFromMemory (BinaryData::panel_png, BinaryData::panel_pngSize)),
      knob (citySize),
      readout (citySize),
      skyline (knob.getShownPercent(), isBypassed()),
      bypassSwitch (bypass),
      displayRefresh (this, [this] (double timestampSeconds) { tick (timestampSeconds); })
{
    // The panel covers every pixel, so nothing behind the editor needs drawing.
    setOpaque (true);
    setResizable (false, false);
    setSize (UiSpec::windowWidth, UiSpec::windowHeight);

    // JUCE moves focus before the clicked control acts, so a click anywhere in the window takes focus from
    // the readout and commits its entry first: a knob drag then starts from the typed value. Keys nobody
    // handles still pass up to the host, as before.
    setWantsKeyboardFocus (true);
    knob.onDoubleClick = [this] { readout.beginEditing(); };

    addAndMakeVisible (knob);
    addAndMakeVisible (readout);
    addAndMakeVisible (skyline);
    addAndMakeVisible (bypassSwitch);
}

void BigCityAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImage (panel, UiSpec::panelBounds);
}

void BigCityAudioProcessorEditor::tick (double timestampSeconds)
{
    if (timestampSeconds - lastTickSeconds < UiSpec::minTickIntervalSeconds)
        return;

    lastTickSeconds = timestampSeconds;
    knob.advanceFrame (timestampSeconds);

    if (++tickCount % UiSpec::ticksPerDisplayUpdate == 0)
        updateDisplays();
}

void BigCityAudioProcessorEditor::updateDisplays()
{
    readout.setDisplayedPercent (citySize.get());

    // The skyline follows the knob's angle on screen, so the two move as one when the knob glides.
    skyline.setState (knob.getShownPercent(), isBypassed());
    bypassSwitch.setDisplayedBypassed (isBypassed());
}

bool BigCityAudioProcessorEditor::isBypassed() const
{
    return bypass.getValue() >= 0.5f;
}

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// The FX ON / BYPASS switch (docs/ui-spec.md "Bypass switch"). It writes the host's bypass parameter itself,
// so the host's own bypass and automation share it, and shows whatever state the editor hands it.
class BypassSwitch final : public juce::Component
{
public:
    explicit BypassSwitch (juce::AudioProcessorParameter& bypassParameter);

    // Called from the editor's repaint timer; repaints only when the state changed.
    void setDisplayedBypassed (bool bypassed);

    void paint (juce::Graphics& g) override;
    bool hitTest (int x, int y) override;

    void mouseUp (const juce::MouseEvent& e) override;

private:
    void drawOverlay (juce::Graphics& g) const;

    juce::AudioProcessorParameter& bypass;
    const juce::Image fxOnImage;
    const juce::Image bypassedImage;
    bool displayedBypassed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BypassSwitch)
};

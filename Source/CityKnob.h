#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// The City Size knob (docs/ui-spec.md "Knob"). It writes citySize itself, wrapping every edit in a
// parameter gesture so hosts record automation cleanly, and shows whatever value the editor hands it.
class CityKnob final : public juce::Component,
                       private juce::Timer
{
public:
    explicit CityKnob (juce::AudioParameterFloat& citySizeParameter);
    ~CityKnob() override;

    // Called from the editor's repaint timer; repaints only when the value moved.
    void setDisplayedPercent (float citySizePercent);

    void paint (juce::Graphics& g) override;
    bool hitTest (int x, int y) override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    // Ends a scroll burst's gesture once the wheel has been quiet for a moment.
    void timerCallback() override;

    void resetToDefault();
    void endDragGesture();
    void endScrollGesture();
    void sendToHost (float citySizePercent);

    void drawRotor (juce::Graphics& g) const;
    void drawOverlay (juce::Graphics& g) const;

    juce::AudioParameterFloat& citySize;
    const juce::Image base;
    const juce::Image rotor;
    float displayedPercent;

    // Unsnapped during a drag or scroll, so slow movements add up across the parameter's 0.1 steps.
    float gesturePercent = 0.0f;
    float lastDragY = 0.0f;
    bool dragGestureActive = false;
    bool scrollGestureActive = false;
    juce::Time lastWheelEventTime;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CityKnob)
};

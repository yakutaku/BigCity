#pragma once

#include "KnobInput.h"
#include "KnobMotion.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The City Size knob (docs/ui-spec.md "Knob"). It writes citySize itself, wrapping every edit in a
// parameter gesture so hosts record automation cleanly, and reads it back on the editor's display tick (D-25).
class CityKnob final : public juce::Component,
                       private juce::Timer
{
public:
    explicit CityKnob (juce::AudioParameterFloat& citySizeParameter);
    ~CityKnob() override;

    // Moves the knob on by one frame, repainting only if its angle changed. The editor's display tick calls
    // it; tests call it directly to step frames without a window.
    void advanceFrame (double timestampSeconds);

    float getShownPercent() const { return motion.getShownPercent(); }

    // Called on a double-click, which opens text entry in the readout (docs/ui-spec.md "Knob" input).
    std::function<void()> onDoubleClick;

    void paint (juce::Graphics& g) override;
    bool hitTest (int x, int y) override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    // Ends a scroll burst once the wheel has been quiet for a moment.
    void timerCallback() override;

    void resetToDefault();
    void endDragGesture();
    void beginScrollGesture();
    void endScrollBurst();
    void sendToHost (float citySizePercent, KnobMotion::Change change);

    static juce::Image composeStaticLayers();
    void drawRotor (juce::Graphics& g, float angleRadians) const;
    void drawOverlay (juce::Graphics& g) const;

    juce::AudioParameterFloat& citySize;
    // The panel behind the knob and knob_base, merged once so each frame starts with a single image.
    const juce::Image staticLayers;
    const juce::Image rotor;
    KnobMotion motion;

    // How the value's next change should reach the screen; the next frame takes it and resets it.
    KnobMotion::Change nextChange = KnobMotion::Change::judgeBySize;

    // Unsnapped during a drag or scroll, so slow movements add up across the parameter's 0.1 steps.
    float gesturePercent = 0.0f;
    float lastDragY = 0.0f;
    bool dragGestureActive = false;
    bool scrollGestureActive = false;
    KnobInput::TrackpadDeadZone trackpadDeadZone;
    juce::Time lastWheelEventTime;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CityKnob)
};

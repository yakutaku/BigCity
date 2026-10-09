#include "BypassSwitch.h"
#include "PluginProcessor.h"
#include "UiSpec.h"

// Drives BypassSwitch's mouse handlers directly, without a window, and checks what reaches the host's bypass.
class BypassSwitchTests final : public juce::UnitTest
{
public:
    BypassSwitchTests() : juce::UnitTest ("BypassSwitch", "BigCity") {}

    void runTest() override
    {
        beginTest ("Hit area: the 128-wide switch, not the LED glow beyond it");
        {
            Fixture fixture;
            const auto lastColumn = UiSpec::BypassSwitch::hitWidth - 1;
            expect (fixture.bypassSwitch.hitTest (0, 0), "top-left");
            expect (fixture.bypassSwitch.hitTest (lastColumn, 111), "bottom-right of the switch");
            expect (! fixture.bypassSwitch.hitTest (lastColumn + 1, 56), "the glow overflow");
        }

        beginTest ("A click toggles bypass on release, as one gesture, and back again");
        {
            Fixture fixture;
            fixture.clickReleasingAt ({ 24, 56 });
            expect (fixture.isBypassed(), "bypassed after one click");
            expectEquals (fixture.gestureBegins, 1);
            expectEquals (fixture.gestureEnds, 1);

            fixture.clickReleasingAt ({ 24, 56 });
            expect (! fixture.isBypassed(), "back on after a second click");
        }

        beginTest ("Releasing outside the switch, including over the glow, cancels the click");
        {
            Fixture fixture;
            fixture.clickReleasingAt ({ 24, 200 });
            fixture.clickReleasingAt ({ UiSpec::BypassSwitch::hitWidth + 2, 56 });
            expect (! fixture.isBypassed());
            expectEquals (fixture.gestureBegins, 0);
        }
    }

private:
    struct Fixture final : private juce::AudioProcessorParameter::Listener
    {
        Fixture() { bypass.addListener (this); }
        ~Fixture() override { bypass.removeListener (this); }

        bool isBypassed() const { return bypass.getValue() >= 0.5f; }

        // A press in the middle of the track, released at the given switch-local point.
        void clickReleasingAt (juce::Point<float> releasePosition)
        {
            const juce::Point<float> pressPosition { 24.0f, 56.0f };
            const auto downTime = juce::Time::getCurrentTime();
            const juce::ModifierKeys pressed { juce::ModifierKeys::leftButtonModifier };

            bypassSwitch.mouseDown (event (pressPosition, pressed, downTime));
            bypassSwitch.mouseUp (event (releasePosition, {}, downTime));
        }

        BigCityAudioProcessor processor;
        juce::AudioProcessorParameter& bypass = *processor.getBypassParameter();
        BypassSwitch bypassSwitch { bypass };
        int gestureBegins = 0, gestureEnds = 0;

    private:
        juce::MouseEvent event (juce::Point<float> position, juce::ModifierKeys mods, juce::Time downTime)
        {
            return { juce::Desktop::getInstance().getMainMouseSource(), position, mods,
                     juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation,
                     juce::MouseInputSource::defaultRotation, juce::MouseInputSource::defaultTiltX,
                     juce::MouseInputSource::defaultTiltY, &bypassSwitch, &bypassSwitch, juce::Time::getCurrentTime(),
                     { 24.0f, 56.0f }, downTime, 1, false };
        }

        void parameterValueChanged (int, float) override {}
        void parameterGestureChanged (int, bool starting) override { ++(starting ? gestureBegins : gestureEnds); }
    };
};

static BypassSwitchTests bypassSwitchTests;

#include "CityKnob.h"
#include "PluginProcessor.h"
#include "ProcessorHarness.h"
#include "UiSpec.h"

// Drives CityKnob's mouse handlers directly, without a window, and checks what reaches the parameter.
class CityKnobTests final : public juce::UnitTest
{
public:
    CityKnobTests() : juce::UnitTest ("CityKnob", "BigCity") {}

    void runTest() override
    {
        beginTest ("Hit area: only the cap, a circle of radius 56 around the knob centre");
        {
            Fixture fixture { 50.0f };
            auto& knob = fixture.knob;
            const auto centre = UiSpec::Knob::centre.toInt();
            const auto radius = (int) UiSpec::Knob::capRadius;

            expect (knob.hitTest (centre.x, centre.y), "centre");
            expect (knob.hitTest (centre.x + radius - 1, centre.y), "just inside the cap, right");
            expect (knob.hitTest (centre.x, centre.y - radius + 1), "just inside the cap, top");
            expect (! knob.hitTest (centre.x + radius + 1, centre.y), "just outside the cap, right");
            expect (! knob.hitTest (centre.x, centre.y - radius - 4), "the 50% tick above the cap");

            // The corner of the old 176 square: well outside the cap's circle.
            const auto knobInset = 8;
            expect (! knob.hitTest (knobInset + 4, knobInset + 4), "corner of the 176 square");
            expect (! knob.hitTest (centre.x + 40, centre.y + 45), "diagonal, outside the circle but inside the square");
        }

        for (const auto startPercent : { 0.0f, 37.3f, 69.6f, 100.0f })
        {
            beginTest ("Option-click resets to exactly 50.0 from " + juce::String (startPercent));
            {
                Fixture fixture { startPercent };
                fixture.click (optionKey, optionKey, {});
                expectExactly50 (fixture);
            }

            beginTest ("Option-click with 1-2 px of movement still resets to exactly 50.0, from " + juce::String (startPercent));
            {
                Fixture fixture { startPercent };
                fixture.click (optionKey, optionKey, { { 0.0f, -1.0f }, { 1.0f, -2.0f } });
                expectExactly50 (fixture);
            }

            beginTest ("Option released mid-click, then 2 px of movement: still exactly 50.0, from " + juce::String (startPercent));
            {
                Fixture fixture { startPercent };
                fixture.click (optionKey, noKeys, { { 0.0f, 2.0f }, { 0.0f, -2.0f } });
                expectExactly50 (fixture);
            }

            beginTest ("A light trackpad brush doesn't move the value or open a gesture, from " + juce::String (startPercent));
            {
                Fixture fixture { startPercent };
                fixture.brushTrackpad();
                expectEquals (fixture.citySize.get(), startPercent);
                expectEquals (fixture.gestures.begins, 0);
            }

            beginTest ("Option-click followed by a trackpad brush stays at exactly 50.0, from " + juce::String (startPercent));
            {
                Fixture fixture { startPercent };
                fixture.click (optionKey, optionKey, {});
                fixture.brushTrackpad();
                expectExactly50 (fixture);
            }

            beginTest ("Double-click doesn't change the value, from " + juce::String (startPercent));
            {
                Fixture fixture { startPercent };
                fixture.doubleClick();
                expectEquals (fixture.citySize.get(), startPercent);
                expectEquals (fixture.gestures.balance(), 0);
            }
        }

        testMotion();
    }

    void testMotion()
    {
        constexpr double halfGlideSeconds = UiSpec::Knob::glideMs / 2000.0;

        beginTest ("Motion: a fast drag (10.0 in one event) follows 1:1 on the next frame");
        {
            Fixture fixture { 50.0f };
            const auto downTime = juce::Time::getCurrentTime();
            fixture.knob.mouseDown (fixture.event (fixture.centre, noKeys, true, downTime, 1, false));
            fixture.knob.mouseDrag (fixture.event (fixture.centre.translated (0.0f, -20.0f), noKeys, true, downTime, 1, true));
            fixture.nextFrame (0.0);
            expectEquals (fixture.citySize.get(), 60.0f);
            expectEquals (fixture.knob.getShownPercent(), 60.0f);
        }

        beginTest ("Motion: Option-click sets the parameter at once, and the knob glides to it");
        {
            Fixture fixture { 0.0f };
            fixture.click (optionKey, optionKey, {});
            expectExactly50 (fixture);

            fixture.nextFrame (0.0);
            expectEquals (fixture.knob.getShownPercent(), 0.0f);
            fixture.nextFrame (halfGlideSeconds);
            expectWithinAbsoluteError (fixture.knob.getShownPercent(), 50.0f * 0.875f, 1.0e-3f);
            fixture.nextFrame (halfGlideSeconds);
            expectEquals (fixture.knob.getShownPercent(), 50.0f);
        }

        beginTest ("Motion: a 1.0 wheel notch glides");
        {
            Fixture fixture { 50.0f };
            juce::MouseWheelDetails wheel {};
            wheel.deltaY = 1.0f;
            fixture.knob.mouseWheelMove (fixture.event (fixture.centre, noKeys, false, {}, 0, false), wheel);

            fixture.nextFrame (0.0);
            fixture.nextFrame (halfGlideSeconds);
            const auto shown = fixture.knob.getShownPercent();
            expect (shown > 50.0f && shown < 51.0f, "mid-glide, got " + juce::String (shown));
        }
    }

private:
    static inline const juce::ModifierKeys noKeys {};
    static inline const juce::ModifierKeys optionKey { juce::ModifierKeys::altModifier };

    // Counts gesture starts and ends, so a test can check every begin has its end.
    struct GestureCounter final : private juce::AudioProcessorParameter::Listener
    {
        explicit GestureCounter (juce::AudioProcessorParameter& parameterToWatch) : parameter (parameterToWatch)
        {
            parameter.addListener (this);
        }

        ~GestureCounter() override { parameter.removeListener (this); }

        int balance() const { return begins - ends; }

        int begins = 0, ends = 0;

    private:
        void parameterValueChanged (int, float) override {}
        void parameterGestureChanged (int, bool starting) override { ++(starting ? begins : ends); }

        juce::AudioProcessorParameter& parameter;
    };

    struct Fixture
    {
        explicit Fixture (float startPercent)
        {
            ProcessorHarness::setParameter (processor, "citySize", startPercent);

            // The knob was built at the default value; let it land on the start value before the test.
            nextFrame (0.0);
            nextFrame (1.0);
        }

        // A display refresh this many seconds after the last one.
        void nextFrame (double secondsLater)
        {
            frameSeconds += secondsLater;
            knob.advanceFrame (frameSeconds);
        }

        // A press at the knob's centre, optional drags by the given offsets, then a release.
        void click (juce::ModifierKeys keysAtDown, juce::ModifierKeys keysWhileMoving,
                    std::initializer_list<juce::Point<float>> movements, int clickCount = 1)
        {
            const auto downTime = juce::Time::getCurrentTime();
            knob.mouseDown (event (centre, keysAtDown, true, downTime, clickCount, false));

            for (const auto offset : movements)
                knob.mouseDrag (event (centre + offset, keysWhileMoving, true, downTime, clickCount, true));

            const auto lastPosition = movements.size() == 0 ? centre : centre + *std::prev (movements.end());
            knob.mouseUp (event (lastPosition, keysWhileMoving, false, downTime, clickCount, movements.size() != 0));
        }

        // The order JUCE delivers a double-click in: down, up, down, double-click, up.
        void doubleClick()
        {
            click (noKeys, noKeys, {}, 1);

            const auto downTime = juce::Time::getCurrentTime();
            knob.mouseDown (event (centre, noKeys, true, downTime, 2, false));
            knob.mouseDoubleClick (event (centre, noKeys, true, downTime, 2, false));
            knob.mouseUp (event (centre, noKeys, false, downTime, 2, false));
        }

        // Lots of tiny trackpad movements that add up to just under the dead zone, as a resting finger makes.
        void brushTrackpad()
        {
            constexpr int eventCount = 20;
            juce::MouseWheelDetails wheel {};
            wheel.isSmooth = true;
            wheel.deltaY = UiSpec::Knob::trackpadDeadZonePixels * 0.9f / UiSpec::Knob::trackpadPixelsPerWheelUnit / (float) eventCount;

            for (int i = 0; i < eventCount; ++i)
                knob.mouseWheelMove (event (centre, noKeys, false, {}, 0, false), wheel);
        }

        juce::MouseEvent event (juce::Point<float> position, juce::ModifierKeys keys, bool buttonDown,
                                juce::Time downTime, int clickCount, bool dragged)
        {
            const auto mods = buttonDown ? keys.withFlags (juce::ModifierKeys::leftButtonModifier) : keys;
            return { juce::Desktop::getInstance().getMainMouseSource(), position, mods,
                     juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation,
                     juce::MouseInputSource::defaultRotation, juce::MouseInputSource::defaultTiltX,
                     juce::MouseInputSource::defaultTiltY, &knob, &knob, juce::Time (++eventTimeMs),
                     centre, downTime, clickCount, dragged };
        }

        BigCityAudioProcessor processor;
        juce::AudioParameterFloat& citySize = processor.getCitySizeParameter();
        GestureCounter gestures { citySize };
        CityKnob knob { citySize };
        juce::Point<float> centre = knob.getLocalBounds().getCentre().toFloat();

        // Each event gets its own time; the knob drops wheel events that repeat a time, as macOS duplicates do.
        juce::int64 eventTimeMs = 0;
        double frameSeconds = 0.0;
    };

    void expectExactly50 (const Fixture& fixture)
    {
        expectEquals (fixture.citySize.get(), 50.0f);
        expect (fixture.gestures.begins == 1, "one gesture per reset, got " + juce::String (fixture.gestures.begins));
        expectEquals (fixture.gestures.balance(), 0);
    }
};

static CityKnobTests cityKnobTests;

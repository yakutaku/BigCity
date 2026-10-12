#include "GestureCounter.h"
#include "PluginProcessor.h"
#include "ProcessorHarness.h"
#include "SevenSegmentReadout.h"

// Drives the readout's key and focus handlers directly, without a window, and checks what reaches the parameter.
class ReadoutEditingTests final : public juce::UnitTest
{
public:
    ReadoutEditingTests() : juce::UnitTest ("ReadoutEditing", "BigCity") {}

    void runTest() override
    {
        beginTest ("Enter commits the typed value in one automation gesture");
        {
            Fixture fixture { 50.0f };
            fixture.readout.beginEditing();
            fixture.type ("72.5");
            fixture.press (juce::KeyPress::returnKey);

            expectWithinAbsoluteError (fixture.citySize.get(), 72.5f, 1.0e-4f);
            expectOneGesture (fixture);
            expect (! fixture.readout.isEditing());
        }

        beginTest ("Enter clamps to 100");
        {
            Fixture fixture { 50.0f };
            fixture.readout.beginEditing();
            fixture.type ("150");
            fixture.press (juce::KeyPress::returnKey);

            expectEquals (fixture.citySize.get(), 100.0f);
            expectOneGesture (fixture);
        }

        beginTest ("Esc cancels: the value stays and no gesture is sent");
        {
            Fixture fixture { 37.3f };
            fixture.readout.beginEditing();
            fixture.type ("72");
            fixture.press (juce::KeyPress::escapeKey);

            expectEquals (fixture.citySize.get(), 37.3f);
            expectEquals (fixture.gestures.begins, 0);
            expect (! fixture.readout.isEditing());
        }

        beginTest ("Clicking elsewhere (focus moves away) commits, like Enter");
        {
            Fixture fixture { 50.0f };
            fixture.readout.beginEditing();
            fixture.type ("30");
            fixture.readout.focusLost (juce::Component::focusChangedByMouseClick);

            expectWithinAbsoluteError (fixture.citySize.get(), 30.0f, 1.0e-4f);
            expectOneGesture (fixture);
            expect (! fixture.readout.isEditing());
        }

        beginTest ("Empty input keeps the old value");
        {
            Fixture fixture { 50.0f };
            fixture.readout.beginEditing();
            fixture.press (juce::KeyPress::backspaceKey);
            fixture.press (juce::KeyPress::returnKey);

            expectEquals (fixture.citySize.get(), 50.0f);
            expectEquals (fixture.gestures.begins, 0);
        }

        beginTest ("Enter without typing, or typing the same value, writes no automation");
        {
            Fixture fixture { 50.0f };
            fixture.readout.beginEditing();
            fixture.press (juce::KeyPress::returnKey);

            fixture.readout.beginEditing();
            fixture.type ("50.01");
            fixture.press (juce::KeyPress::returnKey);

            expectEquals (fixture.citySize.get(), 50.0f);
            expectEquals (fixture.gestures.begins, 0);
        }

        beginTest ("Keys pass to the host when not editing, and Cmd shortcuts pass while editing");
        {
            Fixture fixture { 50.0f };
            expect (! fixture.readout.keyPressed (juce::KeyPress ('7', {}, '7')), "a digit while not editing");
            expect (! fixture.readout.keyPressed (juce::KeyPress (juce::KeyPress::returnKey)), "Enter while not editing");

            fixture.readout.beginEditing();
            expect (! fixture.readout.keyPressed (juce::KeyPress ('s', juce::ModifierKeys::commandModifier, 's')), "Cmd-S");
            expect (fixture.readout.keyPressed (juce::KeyPress ('a', {}, 'a')), "a refused letter is still swallowed");
        }

        beginTest ("A host change while editing doesn't end the entry");
        {
            Fixture fixture { 50.0f };
            fixture.readout.beginEditing();
            fixture.type ("2");
            ProcessorHarness::setParameter (fixture.processor, "citySize", 80.0f);
            fixture.readout.setDisplayedPercent (80.0f);
            fixture.press (juce::KeyPress::returnKey);

            expectWithinAbsoluteError (fixture.citySize.get(), 2.0f, 1.0e-4f);
        }

        beginTest ("Double-click on the readout opens entry");
        {
            Fixture fixture { 50.0f };
            fixture.readout.mouseDoubleClick (juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), {}, {},
                                                                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, &fixture.readout, &fixture.readout,
                                                                juce::Time(), {}, juce::Time(), 2, false));
            expect (fixture.readout.isEditing());
        }
    }

private:
    struct Fixture
    {
        explicit Fixture (float startPercent)
        {
            ProcessorHarness::setParameter (processor, "citySize", startPercent);
        }

        void type (const juce::String& characters)
        {
            for (const auto character : characters)
                readout.keyPressed (juce::KeyPress ((int) character, {}, character));
        }

        void press (int keyCode)
        {
            readout.keyPressed (juce::KeyPress (keyCode));
        }

        BigCityAudioProcessor processor;
        juce::AudioParameterFloat& citySize = processor.getCitySizeParameter();
        GestureCounter gestures { citySize };
        SevenSegmentReadout readout { citySize };
    };

    void expectOneGesture (const Fixture& fixture)
    {
        expectEquals (fixture.gestures.begins, 1);
        expectEquals (fixture.gestures.balance(), 0);
    }
};

static ReadoutEditingTests readoutEditingTests;

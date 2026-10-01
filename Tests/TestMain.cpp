#include <juce_events/juce_events.h>

int main()
{
    // The processor's parameter tree runs timers, so the message manager must exist.
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("BigCity");

    int failureCount = 0;

    for (int i = 0; i < runner.getNumResults(); ++i)
        failureCount += runner.getResult (i)->failures;

    return failureCount == 0 ? 0 : 1;
}

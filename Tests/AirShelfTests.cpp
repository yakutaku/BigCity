#include "AirShelf.h"

#include <juce_audio_basics/juce_audio_basics.h>

class AirShelfTests final : public juce::UnitTest
{
public:
    AirShelfTests() : juce::UnitTest ("AirShelf", "BigCity") {}

    void runTest() override
    {
        beginTest ("At 0 dB the shelf passes audio through bit for bit");
        {
            for (auto sampleRate : { 44100.0, 48000.0, 96000.0, 192000.0 })
            {
                AirShelf shelf;
                shelf.prepare (sampleRate);

                juce::Random random (1);
                bool identical = true;

                for (int i = 0; i < 10000; ++i)
                {
                    const auto input = random.nextFloat() * 2.0f - 1.0f;
                    identical = identical && juce::exactlyEqual (shelf.processSample (i % AirShelf::maxChannels, input), input);
                }

                expect (identical, "output differs from input at " + juce::String (sampleRate) + " Hz");
            }
        }

        beginTest ("At +6 dB the shelf boosts the top and leaves the bass alone");
        {
            constexpr double sampleRate = 48000.0;
            const auto c = AirShelf::makeCoefficients (sampleRate, SoundSpec::airCornerHz, SoundSpec::airShelfQ, 6.0f);

            // At DC z = 1, at Nyquist z = -1, so the response is a ratio of coefficient sums.
            const auto dcGain = (c.b0 + c.b1 + c.b2) / (1.0 + c.a1 + c.a2);
            const auto nyquistGain = (c.b0 - c.b1 + c.b2) / (1.0 - c.a1 + c.a2);

            expectWithinAbsoluteError (juce::Decibels::gainToDecibels (dcGain), 0.0, 1.0e-6);
            expectWithinAbsoluteError (juce::Decibels::gainToDecibels (nyquistGain), 6.0, 1.0e-6);
        }
    }
};

static AirShelfTests airShelfTests;

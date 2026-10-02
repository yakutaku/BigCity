#include "PluginProcessor.h"
#include "ProcessorHarness.h"
#include "SoundSpec.h"

#include <cmath>

using namespace ProcessorHarness;

namespace
{
    juce::AudioBuffer<float> makeNoise (int numChannels, int numSamples)
    {
        juce::AudioBuffer<float> noise (numChannels, numSamples);
        juce::Random random (42);

        for (int channel = 0; channel < numChannels; ++channel)
            for (int i = 0; i < numSamples; ++i)
                noise.setSample (channel, i, random.nextFloat() - 0.5f);

        return noise;
    }

    juce::AudioBuffer<float> makeSine (int numChannels, int numSamples, double frequencyHz, double sampleRate, float amplitude)
    {
        juce::AudioBuffer<float> sine (numChannels, numSamples);

        for (int channel = 0; channel < numChannels; ++channel)
            for (int i = 0; i < numSamples; ++i)
                sine.setSample (channel, i, amplitude * (float) std::sin (juce::MathConstants<double>::twoPi * frequencyHz * i / sampleRate));

        return sine;
    }

    // Largest difference between two buffers, relative to the reference's peak, in dB.
    double residualDb (const juce::AudioBuffer<float>& output, const juce::AudioBuffer<float>& reference, int fromSample)
    {
        float residualPeak = 0.0f, referencePeak = 0.0f;

        for (int channel = 0; channel < output.getNumChannels(); ++channel)
            for (int i = fromSample; i < output.getNumSamples(); ++i)
            {
                residualPeak = std::max (residualPeak, std::abs (output.getSample (channel, i) - reference.getSample (channel, i)));
                referencePeak = std::max (referencePeak, std::abs (reference.getSample (channel, i)));
            }

        return juce::Decibels::gainToDecibels ((double) residualPeak / referencePeak, -std::numeric_limits<double>::infinity());
    }

    float largestStep (const juce::AudioBuffer<float>& audio)
    {
        float step = 0.0f;

        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int i = 1; i < audio.getNumSamples(); ++i)
                step = std::max (step, std::abs (audio.getSample (channel, i) - audio.getSample (channel, i - 1)));

        return step;
    }

    // What the output would be with no crossfade: processed, then dry from one sample to the next.
    float largestStepOfInstantSwitch (const juce::AudioBuffer<float>& processed, const juce::AudioBuffer<float>& dry, int dryFrom, int dryTo)
    {
        auto instantSwitch = processed;

        for (int channel = 0; channel < instantSwitch.getNumChannels(); ++channel)
            instantSwitch.copyFrom (channel, dryFrom, dry, channel, dryFrom, dryTo - dryFrom);

        return largestStep (instantSwitch);
    }

    bool samplesMatch (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, int fromSample, int toSample)
    {
        for (int channel = 0; channel < a.getNumChannels(); ++channel)
            for (int i = fromSample; i < toSample; ++i)
                if (! juce::exactlyEqual (a.getSample (channel, i), b.getSample (channel, i)))
                    return false;

        return true;
    }
}

class ProcessorTests final : public juce::UnitTest
{
public:
    ProcessorTests() : juce::UnitTest ("Processor", "BigCity") {}

    void runTest() override
    {
        testNullAtZero();
        testNullAfterTurningDownToZero();
        testBypassIsClickFree();
        testBypassPathsShareOneCrossfade();
    }

private:
    static constexpr double nullLimitDb = -100.0;   // PRD §4 "Transparent at 0"
    static constexpr int oddBlockSize = 441;        // not a multiple of the 32-sample control interval

    void testNullAtZero()
    {
        beginTest ("At 0 the output nulls against the input (stereo and mono, 44.1/48/96 kHz)");

        for (auto sampleRate : { 44100.0, 48000.0, 96000.0 })
            for (auto numChannels : { 1, 2 })
            {
                BigCityAudioProcessor processor;
                useChannelCount (processor, numChannels);
                setParameter (processor, "citySize", 0.0f);
                processor.prepareToPlay (sampleRate, oddBlockSize);

                const auto input = makeNoise (numChannels, (int) sampleRate);
                auto output = input;
                render (processor, output, oddBlockSize);

                const auto nullDb = residualDb (output, input, 0);
                logMessage ("  " + juce::String (numChannels) + " ch at " + juce::String (sampleRate) + " Hz: residual " + juce::String (nullDb) + " dB");
                expect (nullDb < nullLimitDb, juce::String (numChannels) + " ch at " + juce::String (sampleRate) + " Hz: residual " + juce::String (nullDb) + " dB");
            }
    }

    void testNullAfterTurningDownToZero()
    {
        beginTest ("Turned down from 50 to 0, the output nulls once smoothing has settled");

        constexpr double sampleRate = 48000.0;
        constexpr int settleSamples = 9600;   // 200 ms: well past the 30 ms smoothing

        for (auto numChannels : { 1, 2 })
        {
            BigCityAudioProcessor processor;
            useChannelCount (processor, numChannels);
            setParameter (processor, "citySize", 50.0f);
            processor.prepareToPlay (sampleRate, oddBlockSize);

            const auto input = makeNoise (numChannels, (int) sampleRate);
            auto output = input;
            render (processor, output, oddBlockSize, [&] (int blockIndex)
            {
                if (blockIndex == 10)
                    setParameter (processor, "citySize", 0.0f);
                return false;
            });

            const auto nullDb = residualDb (output, input, 10 * oddBlockSize + settleSamples);
            logMessage ("  " + juce::String (numChannels) + " ch: residual " + juce::String (nullDb) + " dB");
            expect (nullDb < nullLimitDb, juce::String (numChannels) + " ch: residual " + juce::String (nullDb) + " dB");
        }
    }

    void testBypassIsClickFree()
    {
        beginTest ("Bypass on and off is click-free and lands exactly on dry, then processed");

        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 256;
        constexpr int bypassOnBlock = 100, bypassOffBlock = 150;
        constexpr int bypassOnSample = bypassOnBlock * blockSize, bypassOffSample = bypassOffBlock * blockSize;
        const auto crossfadeSamples = (int) (sampleRate * SoundSpec::bypassCrossfadeSeconds);

        // City Size 100 makes the processed signal differ most from the dry one, the hardest case.
        const auto input = makeSine (2, 48000, 1000.0, sampleRate, 0.5f);

        const auto renderWithBypass = [&] (bool toggleBypass)
        {
            BigCityAudioProcessor processor;
            setParameter (processor, "citySize", 100.0f);
            processor.prepareToPlay (sampleRate, blockSize);

            auto output = input;
            render (processor, output, blockSize, [&] (int blockIndex)
            {
                if (toggleBypass && (blockIndex == bypassOnBlock || blockIndex == bypassOffBlock))
                    setParameter (processor, "bypass", blockIndex == bypassOnBlock ? 1.0f : 0.0f);
                return false;
            });
            return output;
        };

        const auto toggled = renderWithBypass (true);
        const auto processed = renderWithBypass (false);

        // A crossfade can't move faster than the faster of the two signals it fades between.
        const auto stepLimit = std::max (largestStep (input), largestStep (processed)) * 1.01f;
        logMessage ("  largest step: toggled " + juce::String (largestStep (toggled)) + ", limit " + juce::String (stepLimit)
                    + ", instant switch " + juce::String (largestStepOfInstantSwitch (processed, input, bypassOnSample, bypassOffSample)));
        expect (largestStep (toggled) <= stepLimit,
                "largest step " + juce::String (largestStep (toggled)) + " exceeds " + juce::String (stepLimit));

        expect (samplesMatch (toggled, input, bypassOnSample + crossfadeSamples, bypassOffSample), "not exactly dry after the fade in");
        expect (samplesMatch (toggled, processed, bypassOffSample + crossfadeSamples, toggled.getNumSamples()), "not exactly processed after the fade out");

        // Proves the step check can catch a click: an instant switch at the same samples must fail it.
        expect (largestStepOfInstantSwitch (processed, input, bypassOnSample, bypassOffSample) > stepLimit,
                "the step check would not have caught an instant switch");
    }

    void testBypassPathsShareOneCrossfade()
    {
        beginTest ("processBlock and processBlockBypassed share one crossfade, never doubling it");

        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 256;
        constexpr int bypassOnBlock = 40, bypassOffBlock = 80;
        const auto input = makeNoise (2, 120 * blockSize);

        const auto inBypassSpan = [] (int blockIndex) { return blockIndex >= bypassOnBlock && blockIndex < bypassOffBlock; };

        // Each way a bypass can arrive: the parameter alone, the bypassed callback alone, or both at once.
        const auto renderBypassedVia = [&] (bool useParameter, bool useBypassedPath)
        {
            BigCityAudioProcessor processor;
            setParameter (processor, "citySize", 75.0f);
            processor.prepareToPlay (sampleRate, blockSize);

            auto output = input;
            render (processor, output, blockSize, [&] (int blockIndex)
            {
                if (useParameter && (blockIndex == bypassOnBlock || blockIndex == bypassOffBlock))
                    setParameter (processor, "bypass", inBypassSpan (blockIndex) ? 1.0f : 0.0f);
                return useBypassedPath && inBypassSpan (blockIndex);
            });
            return output;
        };

        const auto viaParameter = renderBypassedVia (true, false);
        const auto viaBypassedPath = renderBypassedVia (false, true);
        const auto viaBoth = renderBypassedVia (true, true);

        expect (samplesMatch (viaBypassedPath, viaParameter, 0, input.getNumSamples()), "processBlockBypassed fades differently from the parameter");
        expect (samplesMatch (viaBoth, viaParameter, 0, input.getNumSamples()), "parameter plus processBlockBypassed fades differently from either alone");
    }
};

static ProcessorTests processorTests;

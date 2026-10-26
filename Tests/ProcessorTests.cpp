#include "PluginProcessor.h"
#include "ProcessorHarness.h"
#include "SoundSpec.h"

#include <array>
#include <bit>
#include <cmath>
#include <optional>

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

    bool allFinite (const juce::AudioBuffer<float>& audio)
    {
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int i = 0; i < audio.getNumSamples(); ++i)
                if (! std::isfinite (audio.getSample (channel, i)))
                    return false;

        return true;
    }

    // CLAUDE.md "Handle any buffer size and sample rates from 44.1 to 192 kHz".
    constexpr std::array sampleRatesHz { 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 };

    struct BlockSizes
    {
        juce::String name;
        BlockSizeSchedule sizeAt;
        int largest;
    };

    // Sizes that aren't multiples of the 32-sample control interval, and sizes that change from call to
    // call, including the empty blocks some hosts send, as in automation and offline bounce.
    std::vector<BlockSizes> awkwardBlockSizes()
    {
        static constexpr std::array varying { 512, 1, 33, 0, 4097, 7, 31, 256 };
        std::vector<BlockSizes> cases;

        for (auto size : { 1, 7, 31, 33, 511, 4097 })
            cases.push_back ({ juce::String (size), [size] (int) { return size; }, size });

        cases.push_back ({ "varying sizes", [] (int blockIndex) { return varying[(size_t) blockIndex % varying.size()]; }, 4097 });
        return cases;
    }

    juce::MemoryBlock stateOf (juce::AudioProcessor& processor)
    {
        juce::MemoryBlock state;
        processor.getStateInformation (state);
        return state;
    }

    void restore (juce::AudioProcessor& processor, const juce::MemoryBlock& state)
    {
        processor.setStateInformation (state.getData(), (int) state.getSize());
    }

    // Edits a saved state the way a damaged or hand-edited session file might be.
    juce::MemoryBlock editedState (const juce::MemoryBlock& state, const std::function<void (juce::XmlElement&)>& edit)
    {
        auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
        edit (*xml);

        juce::MemoryBlock edited;
        juce::AudioProcessor::copyXmlToBinary (*xml, edited);
        return edited;
    }

    juce::MemoryBlock withSavedValue (const juce::MemoryBlock& state, const juce::String& parameterId, const juce::String& savedValue)
    {
        return editedState (state, [&] (juce::XmlElement& xml) { xml.getChildByAttribute ("id", parameterId)->setAttribute ("value", savedValue); });
    }

    // 64-bit FNV-1a over the output's sample bits: any change to any sample changes it.
    juce::uint64 fingerprint (const juce::AudioBuffer<float>& audio)
    {
        juce::uint64 hash = 14695981039346656037ull;

        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int i = 0; i < audio.getNumSamples(); ++i)
            {
                const auto bits = std::bit_cast<juce::uint32> (audio.getSample (channel, i));

                for (int byte = 0; byte < 4; ++byte)
                    hash = (hash ^ ((bits >> (8 * byte)) & 0xffu)) * 1099511628211ull;
            }

        return hash;
    }

    // The 1.0 reference pass: two seconds of noise at 48 kHz from the restored position (37.5), then
    // automation jumps to 100, 0 and 50, then a sweep from 0 to 100.
    juce::uint64 referencePassFingerprint (BigCityAudioProcessor& processor, int numChannels)
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 441;

        useChannelCount (processor, numChannels);
        processor.prepareToPlay (sampleRate, blockSize);

        auto audio = makeNoise (numChannels, 2 * (int) sampleRate);
        render (processor, audio, blockSize, [&] (BlockPosition block)
        {
            if (block.index == 40)  setParameter (processor, "citySize", 100.0f);
            if (block.index == 80)  setParameter (processor, "citySize", 0.0f);
            if (block.index == 120) setParameter (processor, "citySize", 50.0f);
            if (block.index >= 160) setParameter (processor, "citySize", std::min (100.0f, 2.0f * (float) (block.index - 160)));
            return false;
        });

        return fingerprint (audio);
    }
}

class ProcessorTests final : public juce::UnitTest
{
public:
    ProcessorTests() : juce::UnitTest ("Processor", "BigCity") {}

    void runTest() override
    {
        for (auto voicing : { Voicing::original, Voicing::doubleRange })
        {
            testNullAtZero (voicing);
            testNullAfterTurningDownToZero (voicing);
        }

        testBypassIsClickFree();
        testBypassPathsShareOneCrossfade();
        testHotInputIsNeverHardClipped();
        testStateRoundTrip();
        testUnreadableStateIsIgnored();
        testMissingParameterFallsBackToDefault();
        testOutOfRangeValuesAreClamped();
        testNonNumericValuesNeverReachTheAudio();
        testOneZeroStatePlaysExactlyAsOneZero();
        testNewInstanceIsVoicing2();
        testVoicingRoundTrips();
        testInvalidVoicingIsHandled();
    }

private:
    static constexpr double nullLimitDb = -100.0;   // PRD §4 "Transparent at 0"
    static constexpr int oddBlockSize = 441;        // not a multiple of the 32-sample control interval

    static juce::String nameOf (Voicing voicing)
    {
        return "voicing " + juce::String ((int) voicing);
    }

    void testNullAtZero (Voicing voicing)
    {
        beginTest ("At 0 the output nulls against the input, " + nameOf (voicing) + " (stereo and mono, 44.1–192 kHz, awkward block sizes)");

        for (auto sampleRate : sampleRatesHz)
            for (auto numChannels : { 1, 2 })
            {
                const auto input = makeNoise (numChannels, (int) sampleRate);
                auto worstNullDb = -std::numeric_limits<double>::infinity();

                for (const auto& blockSizes : awkwardBlockSizes())
                {
                    BigCityAudioProcessor processor;
                    useVoicing (processor, voicing);
                    useChannelCount (processor, numChannels);
                    setParameter (processor, "citySize", 0.0f);
                    processor.prepareToPlay (sampleRate, blockSizes.largest);

                    auto output = input;
                    render (processor, output, blockSizes.sizeAt);

                    const auto nullDb = residualDb (output, input, 0);
                    worstNullDb = std::max (worstNullDb, nullDb);
                    expect (nullDb < nullLimitDb, juce::String (numChannels) + " ch at " + juce::String (sampleRate) + " Hz, blocks of "
                                                      + blockSizes.name + ": residual " + juce::String (nullDb) + " dB");
                }

                logMessage ("  " + juce::String (numChannels) + " ch at " + juce::String (sampleRate) + " Hz: worst residual " + juce::String (worstNullDb) + " dB");
            }
    }

    void testNullAfterTurningDownToZero (Voicing voicing)
    {
        beginTest ("Turned down from 50 to 0, the output nulls once smoothing has settled, " + nameOf (voicing));

        constexpr double sampleRate = 48000.0;
        constexpr int settleSamples = 9600;   // 200 ms: well past the 30 ms smoothing

        for (auto numChannels : { 1, 2 })
        {
            BigCityAudioProcessor processor;
            useVoicing (processor, voicing);
            useChannelCount (processor, numChannels);
            setParameter (processor, "citySize", 50.0f);
            processor.prepareToPlay (sampleRate, oddBlockSize);

            const auto input = makeNoise (numChannels, (int) sampleRate);
            auto output = input;
            render (processor, output, oddBlockSize, [&] (BlockPosition block)
            {
                if (block.index == 10)
                    setParameter (processor, "citySize", 0.0f);
                return false;
            });

            const auto nullDb = residualDb (output, input, 10 * oddBlockSize + settleSamples);
            logMessage ("  " + juce::String (numChannels) + " ch: residual " + juce::String (nullDb) + " dB");
            expect (nullDb < nullLimitDb, juce::String (numChannels) + " ch: residual " + juce::String (nullDb) + " dB");
        }
    }

    void testHotInputIsNeverHardClipped()
    {
        beginTest ("A hot input at 100 stays finite and passes above 0 dBFS without being hard clipped (44.1–192 kHz)");

        // PRD §6 "Peaks above 0 dBFS": noise peaking near +12 dBFS, so a clamp anywhere up there would show.
        constexpr float hotInputGain = 8.0f;

        for (auto sampleRate : sampleRatesHz)
            for (auto numChannels : { 1, 2 })
            {
                BigCityAudioProcessor processor;
                useChannelCount (processor, numChannels);
                setParameter (processor, "citySize", 100.0f);
                processor.prepareToPlay (sampleRate, oddBlockSize);

                auto output = makeNoise (numChannels, (int) sampleRate);
                output.applyGain (hotInputGain);
                render (processor, output, oddBlockSize);

                // Hard clipping flattens the tops: many samples sit at exactly the peak. Noise has one.
                const auto peak = output.getMagnitude (0, output.getNumSamples());
                int samplesAtPeak = 0;

                for (int channel = 0; channel < numChannels; ++channel)
                    for (int i = 0; i < output.getNumSamples(); ++i)
                        samplesAtPeak += juce::exactlyEqual (std::abs (output.getSample (channel, i)), peak) ? 1 : 0;

                const auto label = juce::String (numChannels) + " ch at " + juce::String (sampleRate) + " Hz";
                expect (allFinite (output), label + ": output not finite");
                expectGreaterThan (peak, 1.0f, label + ": output held at 0 dBFS");
                expectLessOrEqual (samplesAtPeak, 2, label + ": " + juce::String (samplesAtPeak) + " samples at the peak, so flat-topped");
            }
    }

    void testBypassIsClickFree()
    {
        beginTest ("Bypass on and off is click-free and lands exactly on dry, then processed (44.1–192 kHz, awkward block sizes)");

        for (auto sampleRate : sampleRatesHz)
        {
            // City Size 100 makes the processed signal differ most from the dry one, the hardest case.
            const auto input = makeSine (2, (int) sampleRate, 1000.0, sampleRate, 0.5f);
            auto worstStepRatio = 0.0f;

            for (const auto& blockSizes : awkwardBlockSizes())
            {
                const auto where = juce::String (sampleRate) + " Hz, blocks of " + blockSizes.name + ": ";
                const auto result = renderBypassToggle (input, sampleRate, blockSizes);
                const auto crossfadeSamples = (int) (sampleRate * SoundSpec::bypassCrossfadeSeconds);

                // A crossfade can't move faster than the faster of the two signals it fades between.
                const auto stepLimit = std::max (largestStep (input), largestStep (result.processed)) * 1.01f;
                worstStepRatio = std::max (worstStepRatio, largestStep (result.toggled) / stepLimit);
                expect (largestStep (result.toggled) <= stepLimit,
                        where + "largest step " + juce::String (largestStep (result.toggled)) + " exceeds " + juce::String (stepLimit));

                expect (samplesMatch (result.toggled, input, result.bypassOnSample + crossfadeSamples, result.bypassOffSample),
                        where + "not exactly dry after the fade in");
                expect (samplesMatch (result.toggled, result.processed, result.bypassOffSample + crossfadeSamples, input.getNumSamples()),
                        where + "not exactly processed after the fade out");

                // Proves the step check can catch a click: an instant switch at the same samples must fail it.
                expect (largestStepOfInstantSwitch (result.processed, input, result.bypassOnSample, result.bypassOffSample) > stepLimit,
                        where + "the step check would not have caught an instant switch");
            }

            logMessage ("  " + juce::String (sampleRate) + " Hz: largest step is " + juce::String (worstStepRatio, 3) + " of the limit");
        }
    }

    struct BypassToggleRender
    {
        juce::AudioBuffer<float> toggled, processed;
        int bypassOnSample = -1, bypassOffSample = -1;   // the first block starting at or after each target
    };

    static BypassToggleRender renderBypassToggle (const juce::AudioBuffer<float>& input, double sampleRate, const BlockSizes& blockSizes)
    {
        const auto bypassOnTarget = (int) (0.4 * sampleRate), bypassOffTarget = (int) (0.7 * sampleRate);
        BypassToggleRender result;

        const auto renderWithBypass = [&] (bool toggleBypass)
        {
            BigCityAudioProcessor processor;
            setParameter (processor, "citySize", 100.0f);
            processor.prepareToPlay (sampleRate, blockSizes.largest);

            auto output = input;
            render (processor, output, blockSizes.sizeAt, [&] (BlockPosition block)
            {
                if (! toggleBypass)
                    return false;

                if (result.bypassOnSample < 0 && block.startSample >= bypassOnTarget)
                {
                    setParameter (processor, "bypass", 1.0f);
                    result.bypassOnSample = block.startSample;
                }
                else if (result.bypassOffSample < 0 && block.startSample >= bypassOffTarget)
                {
                    setParameter (processor, "bypass", 0.0f);
                    result.bypassOffSample = block.startSample;
                }

                return false;
            });
            return output;
        };

        result.toggled = renderWithBypass (true);
        result.processed = renderWithBypass (false);
        return result;
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
            render (processor, output, blockSize, [&] (BlockPosition block)
            {
                if (useParameter && (block.index == bypassOnBlock || block.index == bypassOffBlock))
                    setParameter (processor, "bypass", inBypassSpan (block.index) ? 1.0f : 0.0f);
                return useBypassedPath && inBypassSpan (block.index);
            });
            return output;
        };

        const auto viaParameter = renderBypassedVia (true, false);
        const auto viaBypassedPath = renderBypassedVia (false, true);
        const auto viaBoth = renderBypassedVia (true, true);

        expect (samplesMatch (viaBypassedPath, viaParameter, 0, input.getNumSamples()), "processBlockBypassed fades differently from the parameter");
        expect (samplesMatch (viaBoth, viaParameter, 0, input.getNumSamples()), "parameter plus processBlockBypassed fades differently from either alone");
    }

    // CLAUDE.md "Handle invalid states at the boundary (text entry, state restore)".
    void testStateRoundTrip()
    {
        beginTest ("State restores City size and bypass exactly");

        BigCityAudioProcessor saved;
        setParameter (saved, "citySize", 73.4f);
        setParameter (saved, "bypass", 1.0f);

        BigCityAudioProcessor restored;
        restore (restored, stateOf (saved));

        expectWithinAbsoluteError (restored.getCitySizeParameter().get(), 73.4f, 0.001f);
        expect (restored.getBypassParameter()->getValue() >= 0.5f, "bypass not restored");
    }

    void testUnreadableStateIsIgnored()
    {
        beginTest ("State that isn't ours or can't be read leaves the parameters as they were");

        BigCityAudioProcessor source;
        setParameter (source, "citySize", 80.0f);
        const auto valid = stateOf (source);

        juce::MemoryBlock randomBytes (256);
        juce::Random (7).fillBitsRandomly (randomBytes.getData(), randomBytes.getSize());

        const std::vector<std::pair<juce::String, juce::MemoryBlock>> unreadable {
            { "another plugin's tag", editedState (valid, [] (juce::XmlElement& xml) { xml.setTagName ("SomeOtherPlugin"); }) },
            { "truncated to half", juce::MemoryBlock (valid.getData(), valid.getSize() / 2) },
            { "header only", juce::MemoryBlock (valid.getData(), 8) },
            { "random bytes", randomBytes },
            { "empty", juce::MemoryBlock() },
        };

        for (const auto& [name, state] : unreadable)
        {
            BigCityAudioProcessor processor;
            setParameter (processor, "citySize", 30.0f);
            restore (processor, state);

            logMessage ("  " + name + ": City size " + juce::String (processor.getCitySizeParameter().get()));
            expectEquals (processor.getCitySizeParameter().get(), 30.0f, name);
            expect (processor.getBypassParameter()->getValue() < 0.5f, name + ": bypass changed");
        }
    }

    void testMissingParameterFallsBackToDefault()
    {
        beginTest ("A state missing a parameter restores the rest and resets that one to its default");

        BigCityAudioProcessor source;
        setParameter (source, "bypass", 1.0f);
        const auto withoutCitySize = editedState (stateOf (source), [] (juce::XmlElement& xml)
        {
            xml.removeChildElement (xml.getChildByAttribute ("id", "citySize"), true);
        });

        BigCityAudioProcessor processor;
        setParameter (processor, "citySize", 30.0f);
        restore (processor, withoutCitySize);

        expectEquals (processor.getCitySizeParameter().get(), SoundSpec::citySizeDefaultPercent);
        expect (processor.getBypassParameter()->getValue() >= 0.5f, "bypass not restored");
    }

    void testOutOfRangeValuesAreClamped()
    {
        beginTest ("Out-of-range City size is clamped to 0–100");

        BigCityAudioProcessor source;
        const auto valid = stateOf (source);

        for (const auto& [savedValue, expectedPercent] : { std::pair { "250", 100.0f }, std::pair { "-40", 0.0f }, std::pair { "100.05", 100.0f } })
        {
            BigCityAudioProcessor processor;
            restore (processor, withSavedValue (valid, "citySize", savedValue));
            expectEquals (processor.getCitySizeParameter().get(), expectedPercent, savedValue);
        }
    }

    void testNonNumericValuesNeverReachTheAudio()
    {
        beginTest ("nan, inf and non-numeric saved values never reach the audio");

        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 256;
        constexpr float citySizeBeforePercent = 30.0f;
        const auto input = makeSine (2, (int) sampleRate / 10, 1000.0, sampleRate, 0.5f);

        BigCityAudioProcessor source;
        const auto valid = stateOf (source);

        // A non-finite value is treated as missing, so it resets to the default.
        // JUCE reads text that isn't a number as 0, which is already safe.
        constexpr auto defaultPercent = SoundSpec::citySizeDefaultPercent;
        const std::vector<std::pair<juce::String, std::optional<float>>> savedValues {
            { "nan", defaultPercent }, { "NaN", defaultPercent }, { "-nan", defaultPercent },
            { "inf", defaultPercent }, { "-inf", defaultPercent }, { "1e999", defaultPercent },
            { "abc", std::nullopt }, { "", std::nullopt },
        };

        for (const auto& [savedValue, expectedPercent] : savedValues)
        {
            const auto label = "\"" + savedValue + "\"";

            // Restored after prepareToPlay and some audio, as when a host loads a preset during playback.
            BigCityAudioProcessor processor;
            setParameter (processor, "citySize", citySizeBeforePercent);
            processor.prepareToPlay (sampleRate, blockSize);

            auto before = input;
            render (processor, before, blockSize);

            restore (processor, withSavedValue (withSavedValue (valid, "citySize", savedValue), "bypass", savedValue));

            auto after = input;
            render (processor, after, blockSize);

            const auto citySize = processor.getCitySizeParameter().get();
            logMessage ("  " + label + ": City size " + juce::String (citySize) + ", bypass " + juce::String (processor.getBypassParameter()->getValue()));
            expect (std::isfinite (citySize) && citySize >= 0.0f && citySize <= 100.0f, label + ": City size " + juce::String (citySize));
            expect (std::isfinite (processor.getBypassParameter()->getValue()), label + ": bypass not finite");
            expect (allFinite (after), label + ": output not finite");

            if (expectedPercent.has_value())
            {
                expectEquals (citySize, *expectedPercent, label + ": City size not reset to its default");
                expect (processor.getBypassParameter()->getValue() < 0.5f, label + ": bypass not reset to its default (off)");
            }
        }
    }

    // A real state saved by BigCity 1.0.0 (City size 37.5, bypass off): it has no voicing.
    static juce::MemoryBlock oneZeroState()
    {
        juce::MemoryBlock state;
        state.loadFromHexString ("564332217d0000003c3f786d6c2076657273696f6e3d22312e302220656e636f64696e673d225554462d38223f3e20"
                                 "3c426967436974793e3c504152414d2069643d22627970617373222076616c75653d22302e30222f3e3c504152414d"
                                 "2069643d226369747953697a65222076616c75653d2233372e35222f3e3c2f426967436974793e00");
        return state;
    }

    static juce::String savedVoicing (juce::AudioProcessor& processor)
    {
        const auto state = stateOf (processor);
        return juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize())->getStringAttribute ("voicing", "(none)");
    }

    void testOneZeroStatePlaysExactlyAsOneZero()
    {
        beginTest ("A state saved by 1.0 restores as voicing 1 and plays bit for bit what 1.0 played");

        // referencePassFingerprint run by the 1.0.0 code (Source/ unchanged since the v1.0.0 tag) on
        // 2026-10-26, natively on arm64 and under Rosetta on x86_64. The two differ because floating-point
        // contraction differs between the architectures, not because the sound does.
       #if JUCE_ARM
        constexpr juce::uint64 oneZeroMono = 0xf110fbdd25ad7f7full, oneZeroStereo = 0x637b968682885b2aull;
       #else
        constexpr juce::uint64 oneZeroMono = 0xbf6efee2ef994106ull, oneZeroStereo = 0xcbd9690d77aaf796ull;
       #endif

        for (const auto& [numChannels, expected] : { std::pair { 1, oneZeroMono }, std::pair { 2, oneZeroStereo } })
        {
            BigCityAudioProcessor processor;
            restore (processor, oneZeroState());
            expectEquals (savedVoicing (processor), juce::String ("1"));

            // A tuning build's sliders hold the double-precision sound values as floats, so it never
            // matched a shipping build bit for bit. The guarantee is for shipping builds.
           #if ! BIGCITY_TUNING
            expect (referencePassFingerprint (processor, numChannels) == expected, juce::String (numChannels) + " ch output differs from 1.0");
           #else
            juce::ignoreUnused (expected);
           #endif
        }
    }

    void testNewInstanceIsVoicing2()
    {
        beginTest ("A new instance plays and saves voicing 2");

        BigCityAudioProcessor fresh;
        expectEquals (savedVoicing (fresh), juce::String ("2"));

        BigCityAudioProcessor original;
        restore (original, oneZeroState());
        BigCityAudioProcessor doubleRange;
        restore (doubleRange, oneZeroState());
        useVoicing (doubleRange, Voicing::doubleRange);
        expect (referencePassFingerprint (original, 2) != referencePassFingerprint (doubleRange, 2), "voicing 2 sounds the same as voicing 1");
    }

    void testVoicingRoundTrips()
    {
        beginTest ("Save and restore keep the voicing, whichever it is");

        for (auto voicing : { Voicing::original, Voicing::doubleRange })
        {
            BigCityAudioProcessor saved;
            useVoicing (saved, voicing);

            BigCityAudioProcessor restored;
            useVoicing (restored, voicing == Voicing::original ? Voicing::doubleRange : Voicing::original);
            restore (restored, stateOf (saved));
            expectEquals (savedVoicing (restored), juce::String ((int) voicing));

            BigCityAudioProcessor restoredAgain;
            restore (restoredAgain, stateOf (restored));
            expectEquals (savedVoicing (restoredAgain), juce::String ((int) voicing), "second round trip");
        }

        BigCityAudioProcessor processor;
        useVoicing (processor, Voicing::original);
        restore (processor, juce::MemoryBlock (8));
        expectEquals (savedVoicing (processor), juce::String ("1"), "unreadable state changed the voicing");
    }

    // CLAUDE.md "Handle invalid states at the boundary (text entry, state restore)".
    void testInvalidVoicingIsHandled()
    {
        beginTest ("A missing voicing is voicing 1; one this build doesn't know is the newest it does");

        BigCityAudioProcessor source;
        const auto valid = stateOf (source);

        const std::vector<std::pair<juce::String, juce::String>> savedValues {
            { " 1 ", "1" }, { "2", "2" }, { "3", "2" }, { "0", "2" }, { "-1", "2" }, { "1.5", "2" },
            { "abc", "2" }, { "", "2" }, { "nan", "2" },
        };

        for (const auto& [savedValue, expected] : savedValues)
        {
            BigCityAudioProcessor processor;
            restore (processor, editedState (valid, [&] (juce::XmlElement& xml) { xml.setAttribute ("voicing", savedValue); }));
            expectEquals (savedVoicing (processor), expected, "\"" + savedValue + "\"");
        }

        BigCityAudioProcessor processor;
        restore (processor, editedState (valid, [] (juce::XmlElement& xml) { xml.removeAttribute ("voicing"); }));
        expectEquals (savedVoicing (processor), juce::String ("1"), "missing");
    }
};

static ProcessorTests processorTests;

// PRD §6 "Voicing 2" measurements on music: voicing 2's auto gain curve, true peak at 100 on a hot input,
// mono compatibility at 50 and 100 against voicing 1, and loudness-matched renders for listening. A tool
// for building SoundSpec::voicing2AutoGainDb, so it isn't a CTest test.
//
//   VoicingMeasure --vocal=vocal.wav --mix=mix.wav [--render-to=folder]
//
// Auto gain is measured with the current curve in place, so each suggestion is the current value
// corrected by what's still off; the output gain comes after everything else, so one pass converges.
// Paste the suggested line into SoundSpec::voicing2AutoGainDb and run again to confirm.

#include "AutoGain.h"
#include "LoudnessMeasurement.h"
#include "SoundSpec.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>

#include <cstdio>
#include <map>
#include <optional>

namespace
{
    constexpr int blockSize = 512;
    constexpr std::array citySizesForAutoGain { 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f, 100.0f };
    constexpr std::array citySizesForRenders { 25.0f, 50.0f, 75.0f, 100.0f };
    constexpr std::array citySizesForMono { 50.0f, 100.0f };

    // A notch or thinning: a third-octave band that loses this much more than the dry signal does in mono.
    constexpr double monoBandFlagDb = -3.0;

    struct Source
    {
        juce::String name;
        juce::AudioBuffer<float> audio;
    };

    struct Setting
    {
        juce::String name;
        Voicing voicing;
    };

    const std::vector<Setting> allSettings { { "v1", Voicing::original }, { "v2", Voicing::doubleRange } };

    std::optional<Source> read (const juce::String& name, const juce::File& file)
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        const std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

        if (reader == nullptr || reader->numChannels != 2 || ! juce::exactlyEqual (reader->sampleRate, LoudnessMeasurement::sampleRate))
        {
            std::fprintf (stderr, "Can't read a 48 kHz stereo file at %s\n", file.getFullPathName().toRawUTF8());
            return std::nullopt;
        }

        // Followed by silence as long as the plugin's tail, so the renders keep the wet signal's end.
        const auto tailSamples = (int) std::ceil (BigCityAudioProcessor().getTailLengthSeconds() * reader->sampleRate);
        Source source { name, juce::AudioBuffer<float> (2, (int) reader->lengthInSamples + tailSamples) };
        source.audio.clear();
        reader->read (&source.audio, 0, (int) reader->lengthInSamples, 0, true, true);
        return source;
    }

    juce::AudioBuffer<float> process (const juce::AudioBuffer<float>& input, const Setting& setting, float citySizePercent)
    {
        BigCityAudioProcessor processor;
        ProcessorHarness::useVoicing (processor, setting.voicing);
        ProcessorHarness::useChannelCount (processor, input.getNumChannels());
        ProcessorHarness::setParameter (processor, "citySize", citySizePercent);
        processor.prepareToPlay (LoudnessMeasurement::sampleRate, blockSize);

        auto output = input;
        ProcessorHarness::render (processor, output, blockSize);
        return output;
    }

    juce::AudioBuffer<float> monoSum (const juce::AudioBuffer<float>& stereo)
    {
        juce::AudioBuffer<float> mono (1, stereo.getNumSamples());
        mono.clear();
        mono.addFrom (0, 0, stereo, 0, 0, stereo.getNumSamples(), 0.5f);
        mono.addFrom (0, 0, stereo, 1, 0, stereo.getNumSamples(), 0.5f);
        return mono;
    }

    float samplePeak (const juce::AudioBuffer<float>& audio)
    {
        return std::max (audio.getMagnitude (0, 0, audio.getNumSamples()), audio.getMagnitude (1, 0, audio.getNumSamples()));
    }

    // BS.1770-4 Annex 2: the peak of the signal oversampled four times.
    double truePeakDbtp (const juce::AudioBuffer<float>& audio)
    {
        constexpr size_t twoStagesOfTwo = 2;
        juce::dsp::Oversampling<float> oversampling ((size_t) audio.getNumChannels(), twoStagesOfTwo,
                                                     juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false);
        oversampling.initProcessing ((size_t) audio.getNumSamples());

        auto copy = audio;
        juce::dsp::AudioBlock<float> block (copy);
        const auto oversampled = oversampling.processSamplesUp (block);

        float peak = 0.0f;

        for (size_t channel = 0; channel < oversampled.getNumChannels(); ++channel)
            for (size_t i = 0; i < oversampled.getNumSamples(); ++i)
                peak = std::max (peak, std::abs (oversampled.getSample ((int) channel, (int) i)));

        return juce::Decibels::gainToDecibels ((double) peak, -200.0);
    }

    // Long-term power in each third-octave band, 20 Hz to 20 kHz: Welch's method with Hann windows.
    using BandPowers = std::array<double, 31>;

    double bandCentreHz (size_t band)
    {
        return 1000.0 * std::pow (2.0, ((double) band - LoudnessMeasurement::oneKilohertzBand) / 3.0);
    }

    BandPowers bandPowers (const juce::AudioBuffer<float>& audio, int channel)
    {
        constexpr int fftOrder = 13, fftSize = 1 << fftOrder, hop = fftSize / 2;
        juce::dsp::FFT fft (fftOrder);
        juce::dsp::WindowingFunction<float> window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false);
        std::vector<float> frame (2 * fftSize);
        std::vector<double> spectrum (fftSize / 2 + 1, 0.0);

        for (int start = 0; start + fftSize <= audio.getNumSamples(); start += hop)
        {
            std::fill (frame.begin(), frame.end(), 0.0f);
            std::copy_n (audio.getReadPointer (channel, start), fftSize, frame.begin());
            window.multiplyWithWindowingTable (frame.data(), (size_t) fftSize);
            fft.performFrequencyOnlyForwardTransform (frame.data(), true);

            for (size_t bin = 0; bin < spectrum.size(); ++bin)
                spectrum[bin] += (double) frame[bin] * frame[bin];
        }

        BandPowers bands {};

        for (size_t band = 0; band < bands.size(); ++band)
        {
            const auto lowHz = bandCentreHz (band) * std::pow (2.0, -1.0 / 6.0), highHz = bandCentreHz (band) * std::pow (2.0, 1.0 / 6.0);

            for (size_t bin = 1; bin < spectrum.size(); ++bin)
            {
                const auto binHz = (double) bin * LoudnessMeasurement::sampleRate / fftSize;

                if (binHz >= lowHz && binHz < highHz)
                    bands[band] += spectrum[bin];
            }
        }

        return bands;
    }

    // How much each band loses when the stereo signal is summed to mono, in dB: the mono sum's power
    // against the mean of the two channels' powers. 0 dB for identical channels.
    BandPowers monoBandLossDb (const juce::AudioBuffer<float>& stereo)
    {
        const auto mono = bandPowers (monoSum (stereo), 0);
        const auto left = bandPowers (stereo, 0), right = bandPowers (stereo, 1);
        BandPowers loss {};

        for (size_t band = 0; band < loss.size(); ++band)
        {
            const auto channelMean = (left[band] + right[band]) / 2.0;
            loss[band] = channelMean > 0.0 ? 10.0 * std::log10 (std::max (mono[band], 1.0e-30) / channelMean) : 0.0;
        }

        return loss;
    }

    double monoLoudnessLossDb (const juce::AudioBuffer<float>& stereo)
    {
        return LoudnessMeasurement::integratedLufs (monoSum (stereo)) - LoudnessMeasurement::integratedLufs (stereo);
    }

    void printAutoGain (const std::vector<Source>& sources)
    {
        std::printf ("\n## Auto gain on music (gated BS.1770-4, vs bypass; suggestion = current - mean of both files)\n");

        // Voicing 1 keeps its programme-noise curve, so 1.0 sessions stay bit for bit.
        const Setting voicing2 { "v2", Voicing::doubleRange };
        std::printf ("\n%10s %10s %10s %10s %10s\n", "City size", "current", "vocal", "mix", "suggested");
        juce::String suggestedCurve;

        for (size_t point = 0; point < citySizesForAutoGain.size(); ++point)
        {
            const auto citySize = citySizesForAutoGain[point];
            const auto currentDb = AutoGain::gainDbFor (citySize, SoundSpec::voicing2AutoGainDb);
            std::array<double, 2> differenceDb {};

            for (size_t file = 0; file < sources.size(); ++file)
                differenceDb[file] = LoudnessMeasurement::integratedLufs (process (sources[file].audio, voicing2, citySize))
                                   - LoudnessMeasurement::integratedLufs (sources[file].audio);

            // City Size 0 must stay exactly 0 dB so it nulls against bypass.
            const auto suggestedDb = point == 0 ? 0.0 : currentDb - (differenceDb[0] + differenceDb[1]) / 2.0;
            std::printf ("%10.0f %+10.2f %+10.2f %+10.2f %+10.2f\n", citySize, currentDb, differenceDb[0], differenceDb[1], suggestedDb);
            suggestedCurve << (point == 0 ? "" : ", ") << juce::String (suggestedDb, 2) << "f";
        }

        std::printf ("voicing2AutoGainDb { %s }\n", suggestedCurve.toRawUTF8());
    }

    void printTruePeaks (const std::vector<Source>& sources)
    {
        std::printf ("\n## True peak at 100, input normalised to 0 dBFS sample peak (dBTP, 4x oversampled)\n%-16s", "setting");

        for (const auto& source : sources)
            std::printf (" %12s", source.name.toRawUTF8());

        std::vector<juce::AudioBuffer<float>> hotInputs;

        for (const auto& source : sources)
        {
            auto hot = source.audio;
            hot.applyGain (1.0f / samplePeak (hot));
            hotInputs.push_back (hot);
        }

        std::printf ("\n%-16s", "dry");

        for (const auto& hot : hotInputs)
            std::printf (" %+12.2f", truePeakDbtp (hot));

        for (const auto& setting : allSettings)
        {
            std::printf ("\n%-16s", setting.name.toRawUTF8());

            for (const auto& hot : hotInputs)
                std::printf (" %+12.2f", truePeakDbtp (process (hot, setting, 100.0f)));
        }

        std::printf ("\n");
    }

    void printMonoCompatibility (const std::vector<Source>& sources)
    {
        std::printf ("\n## Mono compatibility: loudness change summed to (L+R)/2, relative to the dry file's own change\n");

        for (const auto& source : sources)
        {
            const auto dryLossDb = monoLoudnessLossDb (source.audio);
            const auto dryBandLossDb = monoBandLossDb (source.audio);

            // How far each band of the dry file sits below its loudest band, so a flag in a near-empty band reads as such.
            const auto left = bandPowers (source.audio, 0), right = bandPowers (source.audio, 1);
            BandPowers dryBandLevelDb {};

            for (size_t band = 0; band < dryBandLevelDb.size(); ++band)
                dryBandLevelDb[band] = 10.0 * std::log10 (std::max (left[band] + right[band], 1.0e-30));

            const auto loudestBandDb = *std::max_element (dryBandLevelDb.begin(), dryBandLevelDb.end());
            std::printf ("\n### %s (dry stereo to mono: %+.2f dB)\n%-16s %6s %12s %-s\n", source.name.toRawUTF8(), dryLossDb,
                         "setting", "size", "vs dry (dB)", "worst band vs dry (its dry level below the loudest band); bands flagged at -3 dB or worse");

            for (const auto& setting : allSettings)
                for (auto citySize : citySizesForMono)
                {
                    const auto output = process (source.audio, setting, citySize);
                    const auto bandLossDb = monoBandLossDb (output);
                    size_t worstBand = 0;
                    juce::String flagged;

                    for (size_t band = 0; band < bandLossDb.size(); ++band)
                    {
                        const auto relativeDb = bandLossDb[band] - dryBandLossDb[band];

                        if (relativeDb < bandLossDb[worstBand] - dryBandLossDb[worstBand])
                            worstBand = band;

                        if (relativeDb <= monoBandFlagDb)
                            flagged << " " << juce::String (bandCentreHz (band), 0) << " Hz (" << juce::String (relativeDb, 1) << ")";
                    }

                    std::printf ("%-16s %6.0f %+12.2f %+.2f dB at %.0f Hz (%.0f dB down);%s\n", setting.name.toRawUTF8(), citySize,
                                 monoLoudnessLossDb (output) - dryLossDb,
                                 bandLossDb[worstBand] - dryBandLossDb[worstBand], bandCentreHz (worstBand),
                                 loudestBandDb - dryBandLevelDb[worstBand],
                                 flagged.isEmpty() ? " none" : flagged.toRawUTF8());
                }
        }
    }

    bool write (const juce::AudioBuffer<float>& audio, const juce::File& file)
    {
        file.deleteFile();   // a FileOutputStream appends to an existing file
        std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (file);
        const auto options = juce::AudioFormatWriterOptions{}
                                 .withSampleRate (LoudnessMeasurement::sampleRate)
                                 .withNumChannels (audio.getNumChannels())
                                 .withBitsPerSample (32)
                                 .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
        const auto writer = juce::WavAudioFormat().createWriterFor (stream, options);
        return writer != nullptr && writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
    }

    // Every render is set to its dry file's integrated loudness, so the pick is by character, not level.
    // 32-bit float, so peaks above 0 dBFS stay intact.
    bool render (const std::vector<Source>& sources, const juce::File& folder)
    {
        std::printf ("\n## Renders in %s (each matched to its dry file's loudness)\n", folder.getFullPathName().toRawUTF8());
        folder.createDirectory();

        for (const auto& source : sources)
        {
            const auto targetLufs = LoudnessMeasurement::integratedLufs (source.audio);
            std::map<juce::String, juce::AudioBuffer<float>> renders { { "dry", source.audio } };

            for (const auto& setting : allSettings)
                for (auto citySize : setting.voicing == Voicing::original ? std::vector { 50.0f, 100.0f }
                                                                          : std::vector (citySizesForRenders.begin(), citySizesForRenders.end()))
                {
                    auto output = process (source.audio, setting, citySize);
                    output.applyGain ((float) juce::Decibels::decibelsToGain (targetLufs - LoudnessMeasurement::integratedLufs (output)));
                    renders[setting.name + "-" + juce::String ((int) citySize).paddedLeft ('0', 3)] = output;
                }

            for (const auto& [name, audio] : renders)
            {
                const auto file = folder.getChildFile (source.name + "-" + name + ".wav");

                if (! write (audio, file))
                {
                    std::fprintf (stderr, "Can't write %s\n", file.getFullPathName().toRawUTF8());
                    return false;
                }

                std::printf ("%-40s %+8.2f LUFS  %+7.2f dBTP\n", file.getFileName().toRawUTF8(),
                             LoudnessMeasurement::integratedLufs (audio), truePeakDbtp (audio));
            }

            // Sanity check: voicing 2's 50 is voicing 1's 100, so once loudness-matched they should null.
            const auto& voicing2At50 = renders["v2-050"];
            const auto& voicing1At100 = renders["v1-100"];
            float residualPeak = 0.0f;

            for (int channel = 0; channel < 2; ++channel)
                for (int i = 0; i < voicing2At50.getNumSamples(); ++i)
                    residualPeak = std::max (residualPeak, std::abs (voicing2At50.getSample (channel, i) - voicing1At100.getSample (channel, i)));

            std::printf ("%s: v2 at 50 against v1 at 100: residual %.1f dB below v1's peak\n", source.name.toRawUTF8(),
                         juce::Decibels::gainToDecibels (residualPeak / samplePeak (voicing1At100), -200.0f));
        }

        return true;
    }
}

int main (int argc, char* argv[])
{
    // The processor's parameter tree runs timers, so the message manager must exist.
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const juce::ArgumentList args (argc, argv);
    const auto vocal = read ("vocal", juce::File (args.getValueForOption ("--vocal")));
    const auto mix = read ("mix", juce::File (args.getValueForOption ("--mix")));

    if (! vocal.has_value() || ! mix.has_value())
        return 1;

    const std::vector<Source> sources { *vocal, *mix };
    printAutoGain (sources);
    printTruePeaks (sources);
    printMonoCompatibility (sources);

    if (args.containsOption ("--render-to"))
        return render (sources, juce::File (args.getValueForOption ("--render-to"))) ? 0 : 1;

    return 0;
}

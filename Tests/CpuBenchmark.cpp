// Loads the installed BigCity.vst3 many times and runs every instance on one real-time thread at real-time
// pace, as a host would on a single core: the worst case. Reports the audio thread's load as a share of
// each buffer's time budget, and the whole process's CPU, with or without one editor open. A measuring tool
// for the CPU benchmark, so it isn't a CTest test.
//
//   CpuBenchmark [--instances=30] [--buffer=128] [--rate=48000] [--seconds=20] [--editor] [--sweep] [--city-size=50]
//                [--close-at=s --reopen-at=s] [--plugin=path]
//
// --close-at and --reopen-at close the editor and open it again at those times, as a host does when the
// window is closed and reopened, so memory can be checked in each state.
//
//   CpuBenchmark --render=in.wav --output=out.wav [--city-size=50] [--voicing=A] [--buffer=128] [--plugin=path]
//
// --render skips the benchmark and renders a file through one instance as fast as it can, for listening
// tests, to 32-bit float WAV so nothing above 0 dBFS is cut off. --voicing needs a BIGCITY_TUNING build.

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <sys/resource.h>

#include <algorithm>
#include <chrono>
#include <thread>

namespace
{
    struct Options
    {
        int instances = 30;
        int blockSize = 128;
        double sampleRate = 48000.0;
        double seconds = 20.0;
        bool openEditor = false;
        bool sweep = false;
        float citySizePercent = 50.0f;   // held still unless --sweep
        double closeAtSeconds = 0.0;     // 0: never
        double reopenAtSeconds = 0.0;    // 0: never
        juce::String renderInputPath, renderOutputPath, voicing;   // empty: benchmark, current voicing
        juce::String pluginPath = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                                      .getChildFile ("Library/Audio/Plug-Ins/VST3/BigCity.vst3").getFullPathName();
    };

    Options parseOptions (const juce::ArgumentList& args)
    {
        Options options;
        options.instances = args.containsOption ("--instances") ? args.getValueForOption ("--instances").getIntValue() : options.instances;
        options.blockSize = args.containsOption ("--buffer") ? args.getValueForOption ("--buffer").getIntValue() : options.blockSize;
        options.sampleRate = args.containsOption ("--rate") ? args.getValueForOption ("--rate").getDoubleValue() : options.sampleRate;
        options.seconds = args.containsOption ("--seconds") ? args.getValueForOption ("--seconds").getDoubleValue() : options.seconds;
        options.openEditor = args.containsOption ("--editor");
        options.sweep = args.containsOption ("--sweep");
        options.closeAtSeconds = args.containsOption ("--close-at") ? args.getValueForOption ("--close-at").getDoubleValue() : 0.0;
        options.reopenAtSeconds = args.containsOption ("--reopen-at") ? args.getValueForOption ("--reopen-at").getDoubleValue() : 0.0;
        options.citySizePercent = args.containsOption ("--city-size") ? args.getValueForOption ("--city-size").getFloatValue() : options.citySizePercent;
        options.pluginPath = args.containsOption ("--plugin") ? args.getValueForOption ("--plugin") : options.pluginPath;
        options.renderInputPath = args.getValueForOption ("--render");
        options.renderOutputPath = args.getValueForOption ("--output");
        options.voicing = args.getValueForOption ("--voicing");
        return options;
    }

    double processCpuSeconds()
    {
        rusage usage {};
        getrusage (RUSAGE_SELF, &usage);
        const auto toSeconds = [] (timeval t) { return (double) t.tv_sec + (double) t.tv_usec / 1.0e6; };
        return toSeconds (usage.ru_utime) + toSeconds (usage.ru_stime);
    }

    juce::AudioProcessorParameter* findParameter (juce::AudioPluginInstance& plugin, const juce::String& name)
    {
        for (auto* parameter : plugin.getParameters())
            if (parameter->getName (64) == name)
                return parameter;

        return nullptr;
    }

    juce::AudioProcessorParameter* findCitySize (juce::AudioPluginInstance& plugin)
    {
        return findParameter (plugin, "City size");
    }

    // Processes every instance, block by block, paced to the sample rate like an audio device callback.
    class AudioLoop final : public juce::Thread
    {
    public:
        AudioLoop (const Options& optionsToUse, juce::OwnedArray<juce::AudioPluginInstance>& pluginsToRun)
            : juce::Thread ("BigCity benchmark audio"), options (optionsToUse), plugins (pluginsToRun)
        {
            const auto expectedBlocks = (size_t) (options.seconds * options.sampleRate / options.blockSize) + 64;
            blockSeconds.reserve (expectedBlocks);

            for (auto* plugin : plugins)
                citySizeParameters.push_back (findCitySize (*plugin));
        }

        void run() override
        {
            juce::AudioBuffer<float> source (2, options.blockSize), buffer (2, options.blockSize);
            juce::MidiBuffer midi;
            juce::Random random (1);

            // Noise at about −12 dBFS, so every stage does real work.
            for (int channel = 0; channel < source.getNumChannels(); ++channel)
                for (int i = 0; i < source.getNumSamples(); ++i)
                    source.setSample (channel, i, 0.25f * (random.nextFloat() * 2.0f - 1.0f));

            const auto blockDuration = std::chrono::duration<double> (options.blockSize / options.sampleRate);
            auto nextBlock = std::chrono::steady_clock::now();
            juce::int64 blockIndex = 0;

            while (! threadShouldExit())
            {
                if (options.sweep)
                    sweepCitySize ((double) blockIndex * options.blockSize / options.sampleRate);

                const auto start = std::chrono::steady_clock::now();

                for (auto* plugin : plugins)
                {
                    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                        buffer.copyFrom (channel, 0, source, channel, 0, options.blockSize);

                    plugin->processBlock (buffer, midi);
                }

                const auto end = std::chrono::steady_clock::now();

                if (blockSeconds.size() < blockSeconds.capacity())
                    blockSeconds.push_back (std::chrono::duration<double> (end - start).count());

                ++blockIndex;
                nextBlock += std::chrono::duration_cast<std::chrono::steady_clock::duration> (blockDuration);

                // A late block doesn't build up debt: like a device, the next one comes a block later.
                if (nextBlock < end)
                    nextBlock = end;

                std::this_thread::sleep_until (nextBlock);
            }
        }

        std::vector<double> blockSeconds;

    private:
        // A slow 0–100 sweep, 8 s per cycle, as host automation, so smoothing runs and the editor repaints.
        void sweepCitySize (double seconds)
        {
            constexpr double cycleSeconds = 8.0;
            const auto normalised = (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * seconds / cycleSeconds));

            for (auto* parameter : citySizeParameters)
                if (parameter != nullptr)
                    parameter->setValue (normalised);
        }

        const Options& options;
        juce::OwnedArray<juce::AudioPluginInstance>& plugins;
        std::vector<juce::AudioProcessorParameter*> citySizeParameters;
    };

    // Picks the voicing as a host's text entry would, and checks the plugin took it.
    bool setVoicing (juce::AudioPluginInstance& plugin, const juce::String& voicing)
    {
        auto* parameter = findParameter (plugin, "Tuning: voicing");

        if (parameter == nullptr)
            return false;

        parameter->setValueNotifyingHost (parameter->getValueForText (voicing));
        return parameter->getCurrentValueAsText() == voicing;
    }

    int renderOffline (const Options& options, juce::AudioPluginFormatManager& formatManager, const juce::PluginDescription& type)
    {
        juce::AudioFormatManager audioFormats;
        audioFormats.registerBasicFormats();
        const std::unique_ptr<juce::AudioFormatReader> reader (audioFormats.createReaderFor (juce::File (options.renderInputPath)));

        if (reader == nullptr || reader->numChannels < 1 || reader->numChannels > 2)
        {
            std::cerr << "Can't read a mono or stereo file at " << options.renderInputPath << std::endl;
            return 1;
        }

        const auto numChannels = (int) reader->numChannels;
        juce::AudioBuffer<float> audio (numChannels, (int) reader->lengthInSamples);
        reader->read (&audio, 0, audio.getNumSamples(), 0, true, true);

        juce::String error;
        auto plugin = formatManager.createPluginInstance (type, reader->sampleRate, options.blockSize, error);

        if (plugin == nullptr)
        {
            std::cerr << "The plugin failed to load: " << error << std::endl;
            return 1;
        }

        const auto channelSet = juce::AudioChannelSet::canonicalChannelSet (numChannels);
        juce::AudioProcessor::BusesLayout layout;
        layout.inputBuses.add (channelSet);
        layout.outputBuses.add (channelSet);
        plugin->setBusesLayout (layout);

        if (auto* citySize = findCitySize (*plugin))
            citySize->setValueNotifyingHost (options.citySizePercent / 100.0f);

        if (options.voicing.isNotEmpty() && ! setVoicing (*plugin, options.voicing))
        {
            std::cerr << "This plugin has no voicing " << options.voicing << "; is it a BIGCITY_TUNING build?" << std::endl;
            return 1;
        }

        plugin->prepareToPlay (reader->sampleRate, options.blockSize);
        juce::MidiBuffer midi;

        const auto processInBlocks = [&] (juce::AudioBuffer<float>& buffer)
        {
            for (int start = 0; start < buffer.getNumSamples(); start += options.blockSize)
            {
                juce::AudioBuffer<float> block (buffer.getArrayOfWritePointers(), numChannels, start,
                                                std::min (options.blockSize, buffer.getNumSamples() - start));
                plugin->processBlock (block, midi);
            }
        };

        // A hosted VST3 receives parameter changes with its next block, so silence goes first: the
        // render then starts at the set values instead of gliding to them from the defaults.
        juce::AudioBuffer<float> preRoll (numChannels, (int) (0.1 * reader->sampleRate));
        preRoll.clear();
        processInBlocks (preRoll);
        processInBlocks (audio);
        plugin->releaseResources();

        const juce::File outputFile (options.renderOutputPath);
        outputFile.deleteFile();   // a FileOutputStream appends to an existing file

        std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (outputFile);
        const auto writerOptions = juce::AudioFormatWriterOptions{}
                                       .withSampleRate (reader->sampleRate)
                                       .withNumChannels (numChannels)
                                       .withBitsPerSample (32)
                                       .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
        const auto writer = juce::WavAudioFormat().createWriterFor (stream, writerOptions);

        if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()))
        {
            std::cerr << "Can't write " << options.renderOutputPath << std::endl;
            return 1;
        }

        std::cout << "rendered " << options.renderOutputPath << " (City size " << options.citySizePercent
                  << ", voicing " << (options.voicing.isEmpty() ? "current" : options.voicing) << ")" << std::endl;
        return 0;
    }

    double percentile (std::vector<double> values, double fraction)
    {
        if (values.empty())
            return 0.0;

        const auto index = (size_t) std::min ((double) values.size() - 1.0, fraction * (double) values.size());
        std::nth_element (values.begin(), values.begin() + (std::ptrdiff_t) index, values.end());
        return values[index];
    }
}

int main (int argc, char* argv[])
{
    const juce::ScopedJuceInitialiser_GUI juceInitialiser;
    const auto options = parseOptions (juce::ArgumentList (argc, argv));

    // A plain executable starts as a background process; an editor window needs a regular app to draw.
    juce::Process::setDockIconVisible (options.openEditor);

    juce::AudioPluginFormatManager formatManager;
    juce::addDefaultFormatsToManager (formatManager);

    juce::OwnedArray<juce::PluginDescription> types;
    juce::VST3PluginFormat vst3;
    vst3.findAllTypesForFile (types, options.pluginPath);

    if (types.isEmpty())
    {
        std::cerr << "No VST3 plugin found at " << options.pluginPath << std::endl;
        return 1;
    }

    if (options.renderInputPath.isNotEmpty())
        return renderOffline (options, formatManager, *types.getFirst());

    juce::OwnedArray<juce::AudioPluginInstance> plugins;

    for (int i = 0; i < options.instances; ++i)
    {
        juce::String error;
        auto plugin = formatManager.createPluginInstance (*types.getFirst(), options.sampleRate, options.blockSize, error);

        if (plugin == nullptr)
        {
            std::cerr << "Instance " << i << " failed to load: " << error << std::endl;
            return 1;
        }

        juce::AudioProcessor::BusesLayout stereo;
        stereo.inputBuses.add (juce::AudioChannelSet::stereo());
        stereo.outputBuses.add (juce::AudioChannelSet::stereo());
        plugin->setBusesLayout (stereo);

        if (auto* citySize = findCitySize (*plugin))
            citySize->setValueNotifyingHost (options.citySizePercent / 100.0f);

        plugin->prepareToPlay (options.sampleRate, options.blockSize);
        plugins.add (std::move (plugin));
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor;
    std::unique_ptr<juce::DocumentWindow> window;

    const auto openEditor = [&]
    {
        editor.reset (plugins.getFirst()->createEditorAndMakeActive());
        window = std::make_unique<juce::DocumentWindow> ("BigCity benchmark", juce::Colours::black, 0);
        window->setUsingNativeTitleBar (true);
        window->setContentNonOwned (editor.get(), true);
        window->setVisible (true);

        // In front, so macOS composites every repaint: a covered window can cost less than one in use.
        window->toFront (true);
    };

    // As a host closes a plugin window: the window goes, then the editor is deleted.
    const auto closeEditor = [&]
    {
        window.reset();
        editor.reset();
    };

    if (options.openEditor)
        openEditor();

    AudioLoop audioLoop (options, plugins);
    const auto blockMs = 1000.0 * options.blockSize / options.sampleRate;
    const auto cpuBefore = processCpuSeconds();
    const auto wallBefore = juce::Time::getMillisecondCounterHiRes();

    audioLoop.startRealtimeThread (juce::Thread::RealtimeOptions{}.withPeriodMs (blockMs));
    // Runs the message loop until this many seconds after the start, so editor changes land on time.
    const auto runUntil = [wallBefore] (double seconds)
    {
        const auto remainingMs = seconds * 1000.0 - (juce::Time::getMillisecondCounterHiRes() - wallBefore);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (std::max (0, (int) remainingMs));
    };

    if (options.openEditor && options.closeAtSeconds > 0.0)
    {
        runUntil (options.closeAtSeconds);
        closeEditor();
        std::cout << "editor closed at " << options.closeAtSeconds << " s" << std::endl;

        if (options.reopenAtSeconds > options.closeAtSeconds)
        {
            runUntil (options.reopenAtSeconds);
            openEditor();
            std::cout << "editor reopened at " << options.reopenAtSeconds << " s" << std::endl;
        }
    }

    runUntil (options.seconds);
    audioLoop.stopThread (2000);

    const auto wallSeconds = (juce::Time::getMillisecondCounterHiRes() - wallBefore) / 1000.0;
    const auto cpuPercentOfOneCore = 100.0 * (processCpuSeconds() - cpuBefore) / wallSeconds;

    closeEditor();

    const auto& blocks = audioLoop.blockSeconds;
    const auto budgetSeconds = blockMs / 1000.0;
    const auto toLoad = [budgetSeconds] (double seconds) { return 100.0 * seconds / budgetSeconds; };
    double totalSeconds = 0.0;

    for (const auto seconds : blocks)
        totalSeconds += seconds;

    const auto overruns = std::count_if (blocks.begin(), blocks.end(), [budgetSeconds] (double s) { return s > budgetSeconds; });

    std::cout << juce::String::formatted ("instances %d, %.0f Hz, buffer %d (%.2f ms), editor %s, %s, %.0f s, %d blocks\n",
                                          options.instances, options.sampleRate, options.blockSize, blockMs,
                                          options.openEditor ? "open" : "closed", options.sweep ? juce::String ("sweeping City size").toRawUTF8() : ("City size " + juce::String (options.citySizePercent)).toRawUTF8(),
                                          wallSeconds, (int) blocks.size())
              << juce::String::formatted ("  audio thread load (all instances, one core): mean %.1f%%, p99 %.1f%%, max %.1f%% of the buffer; %d overruns\n",
                                          toLoad (totalSeconds / (double) std::max<size_t> (1, blocks.size())),
                                          toLoad (percentile (blocks, 0.99)), toLoad (percentile (blocks, 1.0)), (int) overruns)
              << juce::String::formatted ("  per instance: mean %.2f%% of the buffer\n",
                                          toLoad (totalSeconds / (double) std::max<size_t> (1, blocks.size())) / options.instances)
              << juce::String::formatted ("  process CPU (audio + UI): %.1f%% of one core\n", cpuPercentOfOneCore);

    return 0;
}

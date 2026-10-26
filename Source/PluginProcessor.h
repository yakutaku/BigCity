#pragma once

#include "AirShelf.h"
#include "BypassCrossfade.h"
#include "CityMacro.h"
#include "MicroShifter.h"
#include "SoundTuning.h"
#include "Voicing.h"
#include "WetDrive.h"
#include "WetFilter.h"

#if BIGCITY_TUNING
 #include "TuningParameters.h"
#endif

#include <juce_audio_utils/juce_audio_utils.h>

class BigCityAudioProcessor final : public juce::AudioProcessor
{
public:
    BigCityAudioProcessor();

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    using AudioProcessor::processBlock;
    using AudioProcessor::processBlockBypassed;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    // The editor's knob and readout read and write citySize through this.
    juce::AudioParameterFloat& getCitySizeParameter() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void renderBlock (juce::AudioBuffer<float>& buffer, bool bypassRequested);
    // Gains that ramp across each chunk rather than stepping between chunks.
    struct ChunkGains
    {
        float wet = 0.0f;
        float output = 1.0f;
    };

    void renderChunk (juce::AudioBuffer<float>& buffer, int chunkStart, int chunkLength, ChunkGains gainsAtChunkStart);
    void applyCitySize (float citySizePercent, Voicing voicingNow, const SoundTuning& tuning);
    void applyWetPathTuning (const SoundTuning& tuning);
    SoundTuning currentTuning() const;
    Voicing currentVoicing() const;

    juce::AudioProcessorValueTreeState parameters;
    juce::AudioParameterBool* bypassParameter = nullptr;
    juce::AudioParameterFloat* citySizeParameter = nullptr;
    std::atomic<float>* citySizeValue = nullptr;

    // Set on the message thread when state is restored, read by the audio thread; mirrors the saved state.
    std::atomic<Voicing> voicing { newestVoicing };
    static_assert (std::atomic<Voicing>::is_always_lock_free);

   #if BIGCITY_TUNING
    TuningParameters::Reader tuningParameters { parameters };
   #endif

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> citySizeSmoothed;
    AirShelf airShelf;
    MicroShifter microShifter;
    WetFilter wetFilter;
    WetDrive wetDrive;
    BypassCrossfade bypassCrossfade;
    ChunkGains gains;   // at the end of the last rendered chunk

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigCityAudioProcessor)
};

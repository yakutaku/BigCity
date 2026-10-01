#pragma once

#include "AirShelf.h"
#include "BypassCrossfade.h"
#include "CityMacro.h"
#include "MicroShifter.h"

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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void renderBlock (juce::AudioBuffer<float>& buffer, bool bypassRequested);
    void renderChunk (juce::AudioBuffer<float>& buffer, int chunkStart, int chunkLength, float wetGainAtChunkStart);
    void applyCitySettings (const CitySettings& settings);

    juce::AudioProcessorValueTreeState parameters;
    juce::AudioParameterBool* bypassParameter = nullptr;
    std::atomic<float>* citySizeValue = nullptr;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> citySizeSmoothed;
    AirShelf airShelf;
    MicroShifter microShifter;
    BypassCrossfade bypassCrossfade;
    float wetGain = 0.0f;   // at the end of the last rendered chunk

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigCityAudioProcessor)
};

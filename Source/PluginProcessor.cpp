#include "PluginProcessor.h"
#include "SoundSpec.h"

namespace
{
    // Parameter IDs are frozen after the first beta: renaming one breaks saved sessions.
    constexpr auto citySizeId = "citySize";
    constexpr auto bypassId = "bypass";

    // Bumped only when a parameter's meaning changes, so hosts can migrate old sessions.
    constexpr int parameterVersion = 1;
}

BigCityAudioProcessor::BigCityAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "BigCity", createParameterLayout())
{
    bypassParameter = dynamic_cast<juce::AudioParameterBool*> (parameters.getParameter (bypassId));
    citySizeValue = parameters.getRawParameterValue (citySizeId);
    jassert (bypassParameter != nullptr && citySizeValue != nullptr);
}

juce::AudioProcessorValueTreeState::ParameterLayout BigCityAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    using namespace SoundSpec;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { citySizeId, parameterVersion },
        "City size",
        juce::NormalisableRange<float> { citySizeMinPercent, citySizeMaxPercent, citySizeStepPercent },
        citySizeDefaultPercent,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { bypassId, parameterVersion },
        "Bypass",
        false));

   #if BIGCITY_TUNING
    TuningParameters::addTo (layout);
   #endif

    return layout;
}

SoundTuning BigCityAudioProcessor::currentTuning() const
{
   #if BIGCITY_TUNING
    return tuningParameters.read();
   #else
    return {};
   #endif
}

void BigCityAudioProcessor::prepareToPlay (double sampleRate, int)
{
    airShelf.prepare (sampleRate);
    microShifter.prepare (sampleRate);
    bypassCrossfade.prepare (sampleRate, bypassParameter->get());

    // Start at the current knob position so loading a session doesn't sweep up from 0.
    citySizeSmoothed.reset (sampleRate, SoundSpec::citySizeSmoothingSeconds);
    citySizeSmoothed.setCurrentAndTargetValue (citySizeValue->load());
    applyCitySettings (CityMacro::map (citySizeSmoothed.getCurrentValue(), currentTuning()));
}

void BigCityAudioProcessor::releaseResources()
{
}

bool BigCityAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();

    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == output;
}

void BigCityAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    renderBlock (buffer, bypassParameter->get());
}

void BigCityAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // JUCE's wrappers only call this for plugins without a bypass parameter, so hosts never should.
    // If one does, it goes through processBlock's path and shares its single crossfade, so a fade
    // can't be applied twice or restarted.
    renderBlock (buffer, true);
}

void BigCityAudioProcessor::renderBlock (juce::AudioBuffer<float>& buffer, bool bypassRequested)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    if (getTotalNumInputChannels() == 0)
        return;

    bypassCrossfade.setBypassed (bypassRequested);
    citySizeSmoothed.setTargetValue (citySizeValue->load());

    const auto numSamples = buffer.getNumSamples();

    for (int chunkStart = 0; chunkStart < numSamples; chunkStart += SoundSpec::controlIntervalSamples)
    {
        const auto chunkLength = std::min (SoundSpec::controlIntervalSamples, numSamples - chunkStart);
        const auto wetGainAtChunkStart = wetGain;

        applyCitySettings (CityMacro::map (citySizeSmoothed.skip (chunkLength), currentTuning()));
        renderChunk (buffer, chunkStart, chunkLength, wetGainAtChunkStart);
    }
}

void BigCityAudioProcessor::applyCitySettings (const CitySettings& settings)
{
    airShelf.update (settings.airGainDb, settings.airCornerHz, settings.airShelfQ);
    microShifter.setDetuneCents (settings.detuneCents);
    wetGain = juce::Decibels::decibelsToGain (settings.wetLevelDb, -std::numeric_limits<float>::infinity());
}

void BigCityAudioProcessor::renderChunk (juce::AudioBuffer<float>& buffer, int chunkStart, int chunkLength, float wetGainAtChunkStart)
{
    // The wet level ramps across the chunk so it never steps; the shelf and detune update per chunk.
    const auto wetGainStep = (wetGain - wetGainAtChunkStart) / (float) chunkLength;
    const auto isStereo = getTotalNumInputChannels() > 1;

    auto* left = buffer.getWritePointer (0, chunkStart);
    auto* right = isStereo ? buffer.getWritePointer (1, chunkStart) : nullptr;

    for (int i = 0; i < chunkLength; ++i)
    {
        const auto wetGainNow = wetGainAtChunkStart + wetGainStep * (float) (i + 1);
        const auto dryProportion = bypassCrossfade.nextDryProportion();

        const auto dryLeft = left[i];
        const auto dryRight = isStereo ? right[i] : dryLeft;
        const auto wet = microShifter.process (dryLeft, dryRight);

        if (isStereo)
        {
            left[i] = BypassCrossfade::blend (airShelf.processSample (0, dryLeft + wetGainNow * wet.left), dryLeft, dryProportion);
            right[i] = BypassCrossfade::blend (airShelf.processSample (1, dryRight + wetGainNow * wet.right), dryRight, dryProportion);
        }
        else
        {
            // Mono hears what the stereo version sounds like summed to mono.
            const auto monoWet = (wet.left + wet.right) / 2.0f;
            left[i] = BypassCrossfade::blend (airShelf.processSample (0, dryLeft + wetGainNow * monoWet), dryLeft, dryProportion);
        }
    }
}

juce::AudioProcessorEditor* BigCityAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

bool BigCityAudioProcessor::hasEditor() const { return true; }

const juce::String BigCityAudioProcessor::getName() const { return JucePlugin_Name; }

bool BigCityAudioProcessor::acceptsMidi() const { return false; }
bool BigCityAudioProcessor::producesMidi() const { return false; }
bool BigCityAudioProcessor::isMidiEffect() const { return false; }
double BigCityAudioProcessor::getTailLengthSeconds() const
{
    // The micro-shift keeps sounding for its longest delay after the input stops, so offline renders keep it.
    return (SoundSpec::preDelayRightMs + SoundSpec::microShiftWindowMs / 2.0) / 1000.0;
}

int BigCityAudioProcessor::getNumPrograms() { return 1; }
int BigCityAudioProcessor::getCurrentProgram() { return 0; }
void BigCityAudioProcessor::setCurrentProgram (int) {}
const juce::String BigCityAudioProcessor::getProgramName (int) { return {}; }
void BigCityAudioProcessor::changeProgramName (int, const juce::String&) {}

void BigCityAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BigCityAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Ignore state that isn't ours rather than replacing the tree with something malformed.
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorParameter* BigCityAudioProcessor::getBypassParameter() const
{
    return bypassParameter;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BigCityAudioProcessor();
}

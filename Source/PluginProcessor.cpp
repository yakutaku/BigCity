#include "PluginProcessor.h"
#include "AutoGain.h"
#include "PluginEditor.h"
#include "SoundSpec.h"

namespace
{
    // Parameter IDs are frozen after the first beta: renaming one breaks saved sessions.
    constexpr auto citySizeId = "citySize";
    constexpr auto bypassId = "bypass";

    // Bumped only when a parameter's meaning changes, so hosts can migrate old sessions.
    constexpr int parameterVersion = 1;

    // A gain moving linearly across a chunk, reaching its end value on the chunk's last sample.
    struct GainRamp
    {
        float start, step;

        GainRamp (float from, float to, int length) : start (from), step ((to - from) / (float) length) {}

        float at (int sampleIndex) const { return start + step * (float) (sampleIndex + 1); }
    };

    // JUCE reads a saved "nan" as NaN, which its range clamp passes straight through to the audio.
    // Such a parameter is dropped, so it resets to its default, as a parameter missing from the state does.
    void removeNonFiniteValues (juce::XmlElement& state)
    {
        constexpr auto valueAttribute = "value";   // where AudioProcessorValueTreeState saves each value
        juce::Array<juce::XmlElement*> nonFinite;

        for (auto* parameter : state.getChildIterator())
            if (! std::isfinite (parameter->getDoubleAttribute (valueAttribute)))
                nonFinite.add (parameter);

        for (auto* parameter : nonFinite)
            state.removeChildElement (parameter, true);
    }
}

BigCityAudioProcessor::BigCityAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "BigCity", createParameterLayout())
{
    bypassParameter = dynamic_cast<juce::AudioParameterBool*> (parameters.getParameter (bypassId));
    citySizeParameter = dynamic_cast<juce::AudioParameterFloat*> (parameters.getParameter (citySizeId));
    citySizeValue = parameters.getRawParameterValue (citySizeId);
    jassert (bypassParameter != nullptr && citySizeParameter != nullptr && citySizeValue != nullptr);
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
    // Set before prepare, which starts the wet-path glides at these values rather than sweeping to them.
    const auto tuning = currentTuning();
    applyWetPathTuning (tuning);

    airShelf.prepare (sampleRate);
    microShifter.prepare (sampleRate);
    wetFilter.prepare (sampleRate);
    wetDrive.prepare (sampleRate);
    bypassCrossfade.prepare (sampleRate, bypassParameter->get());

    // Start at the current knob position so loading a session doesn't sweep up from 0.
    citySizeSmoothed.reset (sampleRate, SoundSpec::citySizeSmoothingSeconds);
    citySizeSmoothed.setCurrentAndTargetValue (citySizeValue->load());
    applyCitySize (citySizeSmoothed.getCurrentValue(), tuning);
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

    const auto tuning = currentTuning();
    applyWetPathTuning (tuning);

    const auto numSamples = buffer.getNumSamples();

    for (int chunkStart = 0; chunkStart < numSamples; chunkStart += SoundSpec::controlIntervalSamples)
    {
        const auto chunkLength = std::min (SoundSpec::controlIntervalSamples, numSamples - chunkStart);
        const auto gainsAtChunkStart = gains;

        applyCitySize (citySizeSmoothed.skip (chunkLength), tuning);
        wetFilter.advance (chunkLength);
        renderChunk (buffer, chunkStart, chunkLength, gainsAtChunkStart);
    }
}

void BigCityAudioProcessor::applyCitySize (float citySizePercent, const SoundTuning& tuning)
{
    const auto settings = CityMacro::map (citySizePercent, tuning);
    airShelf.update (settings.airGainDb, settings.airCornerHz, settings.airShelfQ);
    microShifter.setDetuneCents (settings.detuneCents);
    gains.wet = juce::Decibels::decibelsToGain (settings.wetLevelDb, -std::numeric_limits<float>::infinity());

    // 0 dB gives a gain of exactly 1, so City Size 0 still nulls bit for bit.
    gains.output = juce::Decibels::decibelsToGain (AutoGain::gainDbFor (citySizePercent, tuning.voicing));
}

void BigCityAudioProcessor::applyWetPathTuning (const SoundTuning& tuning)
{
    microShifter.setModulation (tuning.wetModulation);
    wetFilter.setCutoffs (tuning.wetFilterCutoffs);
    wetDrive.setDriveDb (tuning.wetDriveDb);
}

void BigCityAudioProcessor::renderChunk (juce::AudioBuffer<float>& buffer, int chunkStart, int chunkLength, ChunkGains gainsAtChunkStart)
{
    // The wet level and auto gain ramp across the chunk so they never step; the shelf and detune update per chunk.
    const GainRamp wetGainRamp { gainsAtChunkStart.wet, gains.wet, chunkLength };
    const GainRamp outputGainRamp { gainsAtChunkStart.output, gains.output, chunkLength };
    const auto isStereo = getTotalNumInputChannels() > 1;

    auto* left = buffer.getWritePointer (0, chunkStart);
    auto* right = isStereo ? buffer.getWritePointer (1, chunkStart) : nullptr;

    // The drive sits between the filters so the low-pass smooths the harmonics it adds. All of it
    // happens before the wet level, so at City Size 0 the wet signal is still exactly silent.
    const auto shapeWet = [this] (int channel, float wetSample, float driveGain)
    {
        const auto driven = WetDrive::saturate (wetFilter.processHighPass (channel, wetSample), driveGain);
        return wetFilter.processLowPass (channel, driven);
    };

    for (int i = 0; i < chunkLength; ++i)
    {
        const auto wetGainNow = wetGainRamp.at (i);
        const auto outputGainNow = outputGainRamp.at (i);
        const auto dryProportion = bypassCrossfade.nextDryProportion();
        const auto driveGain = wetDrive.nextDriveGain();

        const auto processed = [&] (int channel, float dry, float shapedWet)
        {
            const auto mixed = airShelf.processSample (channel, dry + wetGainNow * shapedWet);
            return BypassCrossfade::blend (outputGainNow * mixed, dry, dryProportion);
        };

        const auto dryLeft = left[i];
        const auto dryRight = isStereo ? right[i] : dryLeft;
        const auto wet = microShifter.process (dryLeft, dryRight);

        if (isStereo)
        {
            left[i] = processed (0, dryLeft, shapeWet (0, wet.left, driveGain));
            right[i] = processed (1, dryRight, shapeWet (1, wet.right, driveGain));
        }
        else
        {
            // Mono hears what the stereo version sounds like summed to mono.
            left[i] = processed (0, dryLeft, shapeWet (0, (wet.left + wet.right) / 2.0f, driveGain));
        }
    }
}

juce::AudioProcessorEditor* BigCityAudioProcessor::createEditor()
{
   #if BIGCITY_TUNING
    // The tuning parameters have no custom UI; the generic editor is the only way to reach them.
    return new juce::GenericAudioProcessorEditor (*this);
   #else
    return new BigCityAudioProcessorEditor (*this);
   #endif
}

bool BigCityAudioProcessor::hasEditor() const { return true; }

const juce::String BigCityAudioProcessor::getName() const { return JucePlugin_Name; }

bool BigCityAudioProcessor::acceptsMidi() const { return false; }
bool BigCityAudioProcessor::producesMidi() const { return false; }
bool BigCityAudioProcessor::isMidiEffect() const { return false; }
double BigCityAudioProcessor::getTailLengthSeconds() const
{
    // The micro-shift keeps sounding for its longest delay after the input stops, so offline renders keep it.
    return (SoundSpec::preDelayRightMs + SoundSpec::microShiftWindowMs / 2.0 + MicroShifter::maxModulationDepthMs) / 1000.0;
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
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    removeNonFiniteValues (*xml);
    parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorParameter* BigCityAudioProcessor::getBypassParameter() const
{
    return bypassParameter;
}

juce::AudioParameterFloat& BigCityAudioProcessor::getCitySizeParameter() const
{
    return *citySizeParameter;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BigCityAudioProcessor();
}

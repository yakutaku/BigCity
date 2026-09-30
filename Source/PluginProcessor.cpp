#include "PluginProcessor.h"

namespace
{
    // Parameter IDs are frozen after the first beta: renaming one breaks saved sessions.
    constexpr auto citySizeId = "citySize";
    constexpr auto bypassId = "bypass";

    // Bumped only when a parameter's meaning changes, so hosts can migrate old sessions.
    constexpr int parameterVersion = 1;

    constexpr float citySizeMinPercent = 0.0f;
    constexpr float citySizeMaxPercent = 100.0f;
    constexpr float citySizeStepPercent = 0.1f;
    constexpr float citySizeDefaultPercent = 50.0f; // PRD §6 "Default"
}

BigCityAudioProcessor::BigCityAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "BigCity", createParameterLayout())
{
    bypassParameter = dynamic_cast<juce::AudioParameterBool*> (parameters.getParameter (bypassId));
    jassert (bypassParameter != nullptr);
}

juce::AudioProcessorValueTreeState::ParameterLayout BigCityAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

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

    return layout;
}

void BigCityAudioProcessor::prepareToPlay (double, int)
{
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
    juce::ScopedNoDenormals noDenormals;

    // Pass-through: input channels are left untouched; only outputs without a matching input are silenced.
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
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
double BigCityAudioProcessor::getTailLengthSeconds() const { return 0.0; }

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

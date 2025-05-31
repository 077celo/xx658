#include "PluginProcessor.h"
#include "PluginEditor.h"

GUNDAM_PluginAudioProcessor::GUNDAM_PluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize parameter pointers
    masterGain = apvts.getRawParameterValue("MASTER_GAIN");
    masterMix = apvts.getRawParameterValue("MASTER_MIX");
}

GUNDAM_PluginAudioProcessor::~GUNDAM_PluginAudioProcessor()
{
}

const juce::String GUNDAM_PluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GUNDAM_PluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool GUNDAM_PluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool GUNDAM_PluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double GUNDAM_PluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GUNDAM_PluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int GUNDAM_PluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GUNDAM_PluginAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String GUNDAM_PluginAudioProcessor::getProgramName(int index)
{
    return {};
}

void GUNDAM_PluginAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void GUNDAM_PluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialize all sound generators
    hydraulicGen.prepare(sampleRate, samplesPerBlock);
    servoGen.prepare(sampleRate, samplesPerBlock);
    metalImpactGen.prepare(sampleRate, samplesPerBlock);
    gearGrindGen.prepare(sampleRate, samplesPerBlock);
    samplePlayer.prepare(sampleRate, samplesPerBlock);

    // Prepare mix buffer
    mixBuffer.setSize(2, samplesPerBlock);
}

void GUNDAM_PluginAudioProcessor::releaseResources()
{
    hydraulicGen.reset();
    servoGen.reset();
    metalImpactGen.reset();
    gearGrindGen.reset();
    samplePlayer.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GUNDAM_PluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void GUNDAM_PluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Clear mix buffer
    mixBuffer.clear();

    // Process each sound generator
    hydraulicGen.processBlock(mixBuffer, midiMessages, apvts);
    servoGen.processBlock(mixBuffer, midiMessages, apvts);
    metalImpactGen.processBlock(mixBuffer, midiMessages, apvts);
    gearGrindGen.processBlock(mixBuffer, midiMessages, apvts);
    samplePlayer.processBlock(mixBuffer, midiMessages, apvts);

    // Apply master gain and mix
    float gain = masterGain->load();
    float mix = masterMix->load();

    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* mixData = mixBuffer.getReadPointer(channel % mixBuffer.getNumChannels());

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float processedSample = mixData[sample] * gain;
            channelData[sample] = channelData[sample] * (1.0f - mix) + processedSample * mix;
        }
    }
}

bool GUNDAM_PluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GUNDAM_PluginAudioProcessor::createEditor()
{
    return new GUNDAM_PluginAudioProcessorEditor(*this);
}

void GUNDAM_PluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GUNDAM_PluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout GUNDAM_PluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Master parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("MASTER_GAIN", "Master Gain",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("MASTER_MIX", "Master Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    // Hydraulic Generator Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("HYDRAULIC_GAIN", "Hydraulic Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("HYDRAULIC_PRESSURE", "Hydraulic Pressure",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 2.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("HYDRAULIC_FLOW", "Hydraulic Flow Rate",
        juce::NormalisableRange<float>(0.1f, 5.0f, 0.1f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>("HYDRAULIC_ENABLE", "Hydraulic Enable", true));

    // Servo Generator Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("SERVO_GAIN", "Servo Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("SERVO_SPEED", "Servo Speed",
        juce::NormalisableRange<float>(1.0f, 100.0f, 1.0f), 20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("SERVO_WHINE", "Servo Whine",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterBool>("SERVO_ENABLE", "Servo Enable", true));

    // Metal Impact Generator Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("METAL_GAIN", "Metal Impact Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("METAL_RESONANCE", "Metal Resonance",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 2.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("METAL_DECAY", "Metal Decay",
        juce::NormalisableRange<float>(0.1f, 5.0f, 0.1f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>("METAL_ENABLE", "Metal Impact Enable", true));

    // Gear Grind Generator Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("GEAR_GAIN", "Gear Grind Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.4f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("GEAR_ROUGHNESS", "Gear Roughness",
        juce::NormalisableRange<float>(0.1f, 2.0f, 0.1f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("GEAR_SPEED", "Gear Speed",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 2.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>("GEAR_ENABLE", "Gear Grind Enable", true));

    // Sample Player Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("SAMPLE_GAIN", "Sample Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.6f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("SAMPLE_PITCH", "Sample Pitch",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>("SAMPLE_ENABLE", "Sample Enable", true));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GUNDAM_PluginAudioProcessor();
}
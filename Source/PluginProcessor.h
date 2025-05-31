// PluginProcessor.h
#pragma once

#include <JuceHeader.h>
#include "AudioEngine/HydraulicHiss.h"
#include "AudioEngine/ServoWhine.h"
#include "AudioEngine/MetalImpact.h"
#include "AudioEngine/GearGrind.h"
#include "AudioEngine/SamplePlayback.h"

class GUNDAM_PluginAudioProcessor : public juce::AudioProcessor
{
public:
    GUNDAM_PluginAudioProcessor();
    ~GUNDAM_PluginAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Public access to parameters and sound generators
    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }

    HydraulicHiss& getHydraulicHiss() { return hydraulicGen; }
    ServoWhine& getServoWhine() { return servoGen; }
    MetalImpact& getMetalImpact() { return metalImpactGen; }
    GearGrind& getGearGrind() { return gearGrindGen; }
    SamplePlayback& getSamplePlayback() { return SamplePlayback; }

private:
    // Parameter management
    juce::AudioProcessorValueTreeState parameters;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Sound generators
    HydraulicHiss hydraulicGen;
    ServoWhine servoGen;
    MetalImpact metalImpactGen;
    GearGrind gearGrindGen;
    SamplePlayback SamplePlayback;

    // Audio processing
    void processMidiEvents(juce::MidiBuffer& midiMessages);
    void handleNoteOn(int midiChannel, int midiNote, float velocity);
    void handleNoteOff(int midiChannel, int midiNote);
    void handleControlChange(int midiChannel, int controllerNumber, int controllerValue);

    // Master controls
    std::atomic<float>* masterGain;
    std::atomic<float>* masterMix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GUNDAM_PluginAudioProcessor)
};

// PluginProcessor.cpp
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
    parameters(*this, nullptr, juce::Identifier("MechaSoundGenerator"), createParameterLayout())
{
    // Initialize parameter pointers
    masterGain = parameters.getRawParameterValue("masterGain");
    masterMix = parameters.getRawParameterValue("masterMix");
}

GUNDAM_PluginAudioProcessor::~GUNDAM_PluginAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout GUNDAM_PluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master controls
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "masterGain", "Master Gain", 0.0f, 1.0f, 0.7f));
    params.push_back(std://make_unique<juce::AudioParameterFloat>(
    "masterMix", "Master Mix", 0.0f, 1.0f, 1.0f));

    // Hydraulic Generator Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "hydraulicGain", "Hydraulic Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "hydraulicPressure", "Hydraulic Pressure", 0.1f, 2.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "hydraulicHiss", "Hydraulic Hiss", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "hydraulicRelease", "Hydraulic Release", 0.1f, 5.0f, 1.0f));

    // Servo Generator Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "servoGain", "Servo Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "servoSpeed", "Servo Speed", 0.1f, 10.0f, 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "servoTension", "Servo Tension", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "servoWhine", "Servo Whine", 100.0f, 8000.0f, 2000.0f));

    // Metal Impact Generator Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "metalGain", "Metal Impact Gain", 0.0f, 1.0f, 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "metalResonance", "Metal Resonance", 0.1f, 2.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "metalDecay", "Metal Decay", 0.1f, 3.0f, 1.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "metalBrightness", "Metal Brightness", 0.0f, 1.0f, 0.7f));

    // Gear Grind Generator Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gearGain", "Gear Grind Gain", 0.0f, 1.0f, 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gearRoughness", "Gear Roughness", 0.0f, 1.0f, 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gearSpeed", "Gear Speed", 0.1f, 5.0f, 1.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gearTorque", "Gear Torque", 0.0f, 1.0f, 0.5f));

    // Sample Player Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sampleGain", "Sample Gain", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "samplePitch", "Sample Pitch", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sampleAttack", "Sample Attack", 0.001f, 1.0f, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sampleRelease", "Sample Release", 0.1f, 5.0f, 0.5f));

    // Macro Controls
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "macro1", "Macro 1 - Movement Intensity", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "macro2", "Macro 2 - Mechanical Stress", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "macro3", "Macro 3 - Impact Force", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "macro4", "Macro 4 - System Load", 0.0f, 1.0f, 0.5f));

    return { params.begin(), params.end() };
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
    hydraulicGen.setSampleRate(sampleRate);
    servoGen.setSampleRate(sampleRate);
    metalImpactGen.setSampleRate(sampleRate);
    gearGrindGen.setSampleRate(sampleRate);
    SamplePlayback.setSampleRate(sampleRate);

    // Prepare generators
    hydraulicGen.prepareToPlay(samplesPerBlock);
    servoGen.prepareToPlay(samplesPerBlock);
    metalImpactGen.prepareToPlay(samplesPerBlock);
    gearGrindGen.prepareToPlay(samplesPerBlock);
    SamplePlayback.prepareToPlay(samplesPerBlock);
}

void GUNDAM_PluginAudioProcessor::releaseResources()
{
    hydraulicGen.releaseResources();
    servoGen.releaseResources();
    metalImpactGen.releaseResources();
    gearGrindGen.releaseResources();
    SamplePlayback.releaseResources();
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

    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Process MIDI events
    processMidiEvents(midiMessages);

    // Update generator parameters from ValueTreeState
    updateGeneratorParameters();

    // Create temporary buffers for each generator
    juce::AudioBuffer<float> hydraulicBuffer(totalNumOutputChannels, buffer.getNumSamples());
    juce::AudioBuffer<float> servoBuffer(totalNumOutputChannels, buffer.getNumSamples());
    juce::AudioBuffer<float> metalBuffer(totalNumOutputChannels, buffer.getNumSamples());
    juce::AudioBuffer<float> gearBuffer(totalNumOutputChannels, buffer.getNumSamples());
    juce::AudioBuffer<float> sampleBuffer(totalNumOutputChannels, buffer.getNumSamples());

    // Clear temporary buffers
    hydraulicBuffer.clear();
    servoBuffer.clear();
    metalBuffer.clear();
    gearBuffer.clear();
    sampleBuffer.clear();

    // Process each sound generator
    hydraulicGen.processBlock(hydraulicBuffer);
    servoGen.processBlock(servoBuffer);
    metalImpactGen.processBlock(metalBuffer);
    gearGrindGen.processBlock(gearBuffer);
    SamplePlayback.processBlock(sampleBuffer);

    // Mix all generators into main buffer
    buffer.clear();

    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* hydraulicData = hydraulicBuffer.getReadPointer(channel);
        auto* servoData = servoBuffer.getReadPointer(channel);
        auto* metalData = metalBuffer.getReadPointer(channel);
        auto* gearData = gearBuffer.getReadPointer(channel);
        auto* sampleData = sampleBuffer.getReadPointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = (hydraulicData[sample] +
                servoData[sample] +
                metalData[sample] +
                gearData[sample] +
                sampleData[sample]) * masterGain->load();
        }
    }
}

void GUNDAM_PluginAudioProcessor::processMidiEvents(juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            handleNoteOn(message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
        }
        else if (message.isNoteOff())
        {
            handleNoteOff(message.getChannel(), message.getNoteNumber());
        }
        else if (message.isControllerOfType(1)) // Mod wheel
        {
            handleControlChange(message.getChannel(), message.getControllerNumber(), message.getControllerValue());
        }
    }
}

void GUNDAM_PluginAudioProcessor::handleNoteOn(int midiChannel, int midiNote, float velocity)
{
    // Map MIDI notes to different sound generators
    // C4 (60) = Hydraulic, D4 (62) = Servo, E4 (64) = Metal Impact, F4 (65) = Gear Grind
    // G4+ = Sample triggers

    switch (midiNote)
    {
    case 60: // C4 - Hydraulic
        hydraulicGen.trigger(velocity);
        break;
    case 62: // D4 - Servo
        servoGen.trigger(velocity);
        break;
    case 64: // E4 - Metal Impact
        metalImpactGen.trigger(velocity);
        break;
    case 65: // F4 - Gear Grind
        gearGrindGen.trigger(velocity);
        break;
    default:
        if (midiNote >= 67) // G4 and above - Sample Player
            SamplePlayback.trigger(midiNote - 67, velocity);
        break;
    }
}

void GUNDAM_PluginAudioProcessor::handleNoteOff(int midiChannel, int midiNote)
{
    // Handle note off events for sustained sounds
    switch (midiNote)
    {
    case 60:
        hydraulicGen.release();
        break;
    case 62:
        servoGen.release();
        break;
    case 65:
        gearGrindGen.release();
        break;
    default:
        if (midiNote >= 67)
            SamplePlayback.release(midiNote - 67);
        break;
    }
}

void GUNDAM_PluginAudioProcessor::handleControlChange(int midiChannel, int controllerNumber, int controllerValue)
{
    float normalizedValue = controllerValue / 127.0f;

    // Map mod wheel to macro controls
    if (controllerNumber == 1)
    {
        // Mod wheel controls movement intensity
        auto* macro1 = parameters.getRawParameterValue("macro1");
        *macro1 = normalizedValue;
    }
}

void GUNDAM_PluginAudioProcessor::updateGeneratorParameters()
{
    // Update Hydraulic Generator
    hydraulicGen.setGain(parameters.getRawParameterValue("hydraulicGain")->load());
    hydraulicGen.setPressure(parameters.getRawParameterValue("hydraulicPressure")->load());
    hydraulicGen.setHiss(parameters.getRawParameterValue("hydraulicHiss")->load());
    hydraulicGen.setRelease(parameters.getRawParameterValue("hydraulicRelease")->load());

    // Update Servo Generator
    servoGen.setGain(parameters.getRawParameterValue("servoGain")->load());
    servoGen.setSpeed(parameters.getRawParameterValue("servoSpeed")->load());
    servoGen.setTension(parameters.getRawParameterValue("servoTension")->load());
    servoGen.setWhineFreq(parameters.getRawParameterValue("servoWhine")->load());

    // Update Metal Impact Generator
    metalImpactGen.setGain(parameters.getRawParameterValue("metalGain")->load());
    metalImpactGen.setResonance(parameters.getRawParameterValue("metalResonance")->load());
    metalImpactGen.setDecay(parameters.getRawParameterValue("metalDecay")->load());
    metalImpactGen.setBrightness(parameters.getRawParameterValue("metalBrightness")->load());

    // Update Gear Grind Generator
    gearGrindGen.setGain(parameters.getRawParameterValue("gearGain")->load());
    gearGrindGen.setRoughness(parameters.getRawParameterValue("gearRoughness")->load());
    gearGrindGen.setSpeed(parameters.getRawParameterValue("gearSpeed")->load());
    gearGrindGen.setTorque(parameters.getRawParameterValue("gearTorque")->load());

    // Update Sample Player
    SamplePlayback.setGain(parameters.getRawParameterValue("sampleGain")->load());
    SamplePlayback.setPitch(parameters.getRawParameterValue("samplePitch")->load());
    SamplePlayback.setAttack(parameters.getRawParameterValue("sampleAttack")->load());
    SamplePlayback.setRelease(parameters.getRawParameterValue("sampleRelease")->load());

    // Apply Macro Controls
    applyMacroControls();
}

void GUNDAM_PluginAudioProcessor::applyMacroControls()
{
    float macro1 = parameters.getRawParameterValue("macro1")->load(); // Movement Intensity
    float macro2 = parameters.getRawParameterValue("macro2")->load(); // Mechanical Stress
    float macro3 = parameters.getRawParameterValue("macro3")->load(); // Impact Force
    float macro4 = parameters.getRawParameterValue("macro4")->load(); // System Load

    // Macro 1 - Movement Intensity affects hydraulic pressure and servo speed
    hydraulicGen.setPressure(hydraulicGen.getPressure() * (0.5f + macro1 * 1.5f));
    servoGen.setSpeed(servoGen.getSpeed() * (0.3f + macro1 * 1.7f));

    // Macro 2 - Mechanical Stress affects gear roughness and metal resonance
    gearGrindGen.setRoughness(gearGrindGen.getRoughness() * (0.2f + macro2 * 1.8f));
    metalImpactGen.setResonance(metalImpactGen.getResonance() * (0.4f + macro2 * 1.6f));

    // Macro 3 - Impact Force affects metal impact gain and sample attack
    metalImpactGen.setGain(metalImpactGen.getGain() * (0.3f + macro3 * 1.7f));
    SamplePlayback.setAttack(SamplePlayback.getAttack() * (1.0f - macro3 * 0.8f));

    // Macro 4 - System Load affects all generators' torque/tension parameters
    servoGen.setTension(servoGen.getTension() * (0.1f + macro4 * 1.9f));
    gearGrindGen.setTorque(gearGrindGen.getTorque() * (0.2f + macro4 * 1.8f));
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
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GUNDAM_PluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GUNDAM_PluginAudioProcessor();
}
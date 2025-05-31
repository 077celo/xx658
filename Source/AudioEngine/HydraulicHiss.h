#pragma once

#include <JuceHeader.h>

class HydraulicHiss
{
public:
    HydraulicHiss();
    ~HydraulicHiss();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
        juce::AudioProcessorValueTreeState& apvts);

private:
    // Audio processing
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Noise generators for hydraulic hiss
    juce::Random random;
    juce::dsp::IIR::Filter<float> lowPassFilter;
    juce::dsp::IIR::Filter<float> highPassFilter;
    juce::dsp::IIR::Filter<float> bandPassFilter;

    // Oscillators for pressure cycling
    float pressurePhase = 0.0f;
    float flowPhase = 0.0f;

    // Envelope for hydraulic activation
    juce::ADSR hydraulicEnvelope;
    juce::ADSR::Parameters envelopeParams;

    // Internal state
    bool isActive = false;
    float currentPressure = 0.0f;
    float currentFlow = 0.0f;

    // Parameter smoothing
    juce::LinearSmoothedValue<float> gainSmoother;
    juce::LinearSmoothedValue<float> pressureSmoother;
    juce::LinearSmoothedValue<float> flowSmoother;

    // MIDI handling
    void processMidiNote(int midiNote, bool isNoteOn, float velocity);

    // Sound generation
    float generateHydraulicHiss();
    float generatePressureCycle();
    float generateFlowNoise();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HydraulicHiss)
};
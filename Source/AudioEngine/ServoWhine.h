#pragma once

#include <JuceHeader.h>

class ServoWhine
{
public:
    ServoWhine();
    ~ServoWhine();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
        juce::AudioProcessorValueTreeState& apvts);

private:
    // Audio processing
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Oscillators for servo whine and motor sounds
    float servoPhase = 0.0f;
    float motorPhase = 0.0f;
    float whinePhase = 0.0f;

    // Filters for servo sound shaping
    juce::dsp::IIR::Filter<float> resonantFilter;
    juce::dsp::IIR::Filter<float> highPassFilter;

    // Envelope for servo activation
    juce::ADSR servoEnvelope;
    juce::ADSR::Parameters envelopeParams;

    // Internal state
    bool isActive = false;
    float currentSpeed = 0.0f;
    float currentWhine = 0.0f;
    float targetSpeed = 0.0f;

    // Parameter smoothing
    juce::LinearSmoothedValue<float> gainSmoother;
    juce::LinearSmoothedValue<float> speedSmoother;
    juce::LinearSmoothedValue<float> whineSmoother;

    // Speed ramping for realistic servo behavior
    juce::LinearSmoothedValue<float> speedRamp;

    // MIDI handling
    void processMidiNote(int midiNote, bool isNoteOn, float velocity);
    void processMidiCC(int ccNumber, float ccValue);

    // Sound generation
    float generateServoWhine();
    float generateMotorNoise();
    float generateGearResonance();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ServoWhine)
};
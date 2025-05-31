#pragma once

#include <JuceHeader.h>

class MetalImpact
{
public:
    MetalImpact();
    ~MetalImpact();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
        juce::AudioProcessorValueTreeState& apvts);

private:
    // Audio processing
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Impact simulation components
    struct ImpactOscillator
    {
        float phase = 0.0f;
        float frequency = 0.0f;
        float amplitude = 0.0f;
        float decay = 0.0f;
        bool active = false;

        void trigger(float freq, float amp, float decayTime);
        float getNextSample(double sampleRate);
        void reset();
    };

    // Multiple oscillators for metallic resonance
    std::array<ImpactOscillator, 8> resonantOscillators;

    // Filters for metal character
    juce::dsp::IIR::Filter<float> resonantFilter1;
    juce::dsp::IIR::Filter<float> resonantFilter2;
    juce::dsp::IIR::Filter<float> highPassFilter;

    // Envelope for impact
    juce::ADSR impactEnvelope;
    juce::ADSR::Parameters envelopeParams;

    // Noise source for initial impact transient
    juce::Random random;

    // Internal state
    bool isActive = false;
    float currentResonance = 0.0f;
    float currentDecay = 0.0f;
    int impactCounter = 0;

    // Parameter smoothing
    juce::LinearSmoothedValue<float> gainSmoother;
    juce::LinearSmoothedValue<float> resonanceSmoother;
    juce::LinearSmoothedValue<float> decaySmoother;

    // Impact timing
    juce::uint32 lastImpactTime = 0;
    const juce::uint32 minImpactInterval = 100; // Minimum ms between impacts

    // MIDI handling
    void processMidiNote(int midiNote, bool isNoteOn, float velocity);

    // Sound generation
    void triggerImpact(float velocity, int noteNumber);
    float generateImpactTransient();
    float generateMetallicResonance();

    // Helper functions
    float getFrequencyForNote(int noteNumber);
    void updateFilterFrequencies();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetalImpact)
};
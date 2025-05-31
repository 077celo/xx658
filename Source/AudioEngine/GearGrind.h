#pragma once

#include <JuceHeader.h>

class GearGrind
{
public:
    GearGrind();
    ~GearGrind();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
        juce::AudioProcessorValueTreeState& apvts);

private:
    // Audio processing
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Oscillators for gear grinding
    float grindPhase1 = 0.0f;
    float grindPhase2 = 0.0f;
    float roughnessPhase = 0.0f;
    float modulationPhase = 0.0f;

    // Filters for gear sound shaping
    juce::dsp::IIR::Filter<float> bandPassFilter1;
    juce::dsp::IIR::Filter<float> bandPassFilter2;
    juce::dsp::IIR::Filter<float> highPassFilter;
    juce::dsp::IIR::Filter<float> notchFilter;

    // Noise generators
    juce::Random random;

    // Envelope for gear activation
    juce::ADSR gearEnvelope;
    juce::ADSR::Parameters envelopeParams;

    // Internal state
    bool isActive = false;
    float currentRoughness = 0.5f;
    float currentSpeed = 2.0f;

    // Parameter smoothing
    juce::LinearSmoothedValue<float> gainSmoother;
    juce::LinearSmoothedValue<float> roughnessSmoother;
    juce::LinearSmoothedValue<float> speedSmoother;

    // Gear interaction simulation
    struct GearTooth
    {
        float position = 0.0f;
        float engagement = 0.0f;
        bool isEngaged = false;
    };

    static constexpr int teethCount = 12;
    std::array<GearTooth, teethCount> gearTeeth;

    // MIDI handling
    void processMidiNote(int midiNote, bool isNoteOn, float velocity);
    void processMidiCC(int ccNumber, float ccValue);

    // Sound generation
    float generateGearGrind();
    float generateGearMesh();
    float generateMetalGrind();
    float generateRoughnessNoise();

    // Gear simulation
    void updateGearTeeth();
    float calculateGearEngagement();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GearGrind)
};
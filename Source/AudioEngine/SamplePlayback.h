#pragma once

#include <JuceHeader.h>

class SamplePlayback
{
public:
    SamplePlayback();
    ~SamplePlayback();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
        juce::AudioProcessorValueTreeState& apvts);

    // Sample management
    bool loadSample(const juce::File& file);
    bool loadSample(const void* data, size_t dataSize);
    void clearSample();
    bool hasSample() const { return sampleBuffer.getNumSamples() > 0; }

private:
    // Audio processing
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Sample data
    juce::AudioBuffer<float> sampleBuffer;
    int sampleLength = 0;
    int numChannels = 0;

    // Playback state
    struct Voice
    {
        bool isActive = false;
        int currentPosition = 0;
        float pitch = 1.0f;
        float gain = 1.0f;
        float velocity = 1.0f;
        int midiNote = -1;

        // Envelope for sample playback
        juce::ADSR envelope;
        bool isReleasing = false;
    };

    static constexpr int maxVoices = 16;
    std::array<Voice, maxVoices> voices;

    // ADSR parameters for sample envelope
    juce::ADSR::Parameters envelopeParams;

    // Parameter smoothing
    juce::LinearSmoothedValue<float> gainSmoother;
    juce::LinearSmoothedValue<float> pitchSmoother;

    // Internal state
    float currentGain = 0.6f;
    float currentPitch = 1.0f;

    // MIDI handling
    void processMidiNote(int midiNote, bool isNoteOn, float velocity);
    void processMidiCC(int ccNumber, float ccValue);

    // Voice management
    int findAvailableVoice();
    void startVoice(int voiceIndex, int midiNote, float velocity, float pitch);
    void stopVoice(int voiceIndex);
    void stopAllVoices();

    // Sample playback
    float getSampleValue(int channel, float position);
    float interpolateSample(int channel, float position);

    // Utility functions
    float noteToFrequency(int midiNote);
    float frequencyToPitchRatio(float targetFreq, float baseFreq = 440.0f);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePlayback)
};
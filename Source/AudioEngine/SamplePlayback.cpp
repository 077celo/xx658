#include "SamplePlayback.h"

SamplePlayback::SamplePlayback()
{
    // Initialize envelope parameters
    envelopeParams.attack = 0.01f;   // Quick attack for samples
    envelopeParams.decay = 0.1f;
    envelopeParams.sustain = 1.0f;   // Full sustain for samples
    envelopeParams.release = 0.2f;

    // Initialize all voices
    for (auto& voice : voices)
    {
        voice.envelope.setParameters(envelopeParams);
    }
}

SamplePlayback::~SamplePlayback()
{
}

void SamplePlayback::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Prepare all voice envelopes
    for (auto& voice : voices)
    {
        voice.envelope.setSampleRate(sampleRate);
    }

    // Initialize smoothers
    gainSmoother.reset(sampleRate, 0.02); // 20ms smoothing
    pitchSmoother.reset(sampleRate, 0.05); // 50ms smoothing
}

void SamplePlayback::reset()
{
    stopAllVoices();

    // Reset all voice envelopes
    for (auto& voice : voices)
    {
        voice.envelope.reset();
    }
}

void SamplePlayback::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
    juce::AudioProcessorValueTreeState& apvts)
{
    // Get parameters
    bool enabled = *apvts.getRawParameterValue("SAMPLE_ENABLE");
    if (!enabled || !hasSample()) return;

    float gain = *apvts.getRawParameterValue("SAMPLE_GAIN");
    float pitch = *apvts.getRawParameterValue("SAMPLE_PITCH");

    // Update smoothed parameters
    gainSmoother.setTargetValue(gain);
    pitchSmoother.setTargetValue(pitch);

    // Process MIDI
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            processMidiNote(message.getNoteNumber(), true, message.getFloatVelocity());
        }
        else if (message.isNoteOff())
        {
            processMidiNote(message.getNoteNumber(), false, 0.0f);
        }
        else if (message.isController())
        {
            processMidiCC(message.getControllerNumber(), message.getControllerValue() / 127.0f);
        }
    }

    // Generate audio from active voices
    auto numSamples = buffer.getNumSamples();
    auto numOutputChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Update smoothed values
        currentGain = gainSmoother.getNextValue();
        currentPitch = pitchSmoother.getNextValue();

        // Process all active voices
        for (auto& voice : voices)
        {
            if (!voice.isActive) continue;

            // Calculate playback position with pitch adjustment
            float playbackSpeed = voice.pitch * currentPitch;
            float currentPos = static_cast<float>(voice.currentPosition);

            // Get envelope value
            float envelopeValue = voice.envelope.getNextSample();

            // Check if voice should stop
            if (voice.isReleasing && envelopeValue <= 0.001f)
            {
                voice.isActive = false;
                continue;
            }

            // Generate sample output for each channel
            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                // Get sample value with interpolation
                float sampleValue = getSampleValue(channel % numChannels, currentPos);

                // Apply voice parameters
                sampleValue *= voice.gain * voice.velocity * currentGain * envelopeValue;

                // Add to output buffer
                buffer.addSample(channel, sample, sampleValue);
            }

            // Advance playback position
            voice.currentPosition += static_cast<int>(playbackSpeed);

            // Check if sample has finished playing
            if (voice.currentPosition >= sampleLength)
            {
                if (!voice.isReleasing)
                {
                    voice.envelope.noteOff();
                    voice.isReleasing = true;
                }
            }
        }
    }
}

void SamplePlayback::processMidiNote(int midiNote, bool isNoteOn, float velocity)
{
    if (isNoteOn)
    {
        // Find available voice
        int voiceIndex = findAvailableVoice();
        if (voiceIndex != -1)
        {
            // Calculate pitch based on MIDI note (C4 = 60 as root note)
            float notePitch = std::pow(2.0f, (midiNote - 60) / 12.0f);
            startVoice(voiceIndex, midiNote, velocity, notePitch);
        }
    }
    else
    {
        // Stop voices with matching MIDI note
        for (auto& voice : voices)
        {
            if (voice.isActive && voice.midiNote == midiNote && !voice.isReleasing)
            {
                voice.envelope.noteOff();
                voice.isReleasing = true;
            }
        }
    }
}

void SamplePlayback::processMidiCC(int ccNumber, float ccValue)
{
    // CC5 controls sample pitch
    if (ccNumber == 5)
    {
        float pitch = 0.25f + ccValue * 3.75f; // 0.25 to 4.0 range
        pitchSmoother.setTargetValue(pitch);
    }
    // CC7 controls sample gain
    else if (ccNumber == 7)
    {
        gainSmoother.setTargetValue(ccValue);
    }
}

bool SamplePlayback::loadSample(const juce::File& file)
{
    if (!file.exists())
        return false;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader == nullptr)
        return false;

    // Read the sample into our buffer
    sampleLength = static_cast<int>(reader->lengthInSamples);
    numChannels = static_cast<int>(reader->numChannels);

    sampleBuffer.setSize(numChannels, sampleLength);
    reader->read(&sampleBuffer, 0, sampleLength, 0, true, true);

    return true;
}

bool SamplePlayback::loadSample(const void* data, size_t dataSize)
{
    juce::MemoryInputStream inputStream(data, dataSize, false);

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inputStream));

    if (reader == nullptr)
        return false;

    // Read the sample into our buffer
    sampleLength = static_cast<int>(reader->lengthInSamples);
    numChannels = static_cast<int>(reader->numChannels);

    sampleBuffer.setSize(numChannels, sampleLength);
    reader->read(&sampleBuffer, 0, sampleLength, 0, true, true);

    return true;
}

void SamplePlayback::clearSample()
{
    stopAllVoices();
    sampleBuffer.clear();
    sampleLength = 0;
    numChannels = 0;
}

int SamplePlayback::findAvailableVoice()
{
    // First, look for completely inactive voice
    for (int i = 0; i < maxVoices; ++i)
    {
        if (!voices[i].isActive)
            return i;
    }

    // If no inactive voice, steal the oldest releasing voice
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].isReleasing)
        {
            voices[i].envelope.reset();
            return i;
        }
    }

    // If all voices are active and not releasing, steal voice 0
    voices[0].envelope.reset();
    return 0;
}

void SamplePlayback::startVoice(int voiceIndex, int midiNote, float velocity, float pitch)
{
    auto& voice = voices[voiceIndex];

    voice.isActive = true;
    voice.isReleasing = false;
    voice.currentPosition = 0;
    voice.pitch = pitch;
    voice.gain = 1.0f;
    voice.velocity = velocity;
    voice.midiNote = midiNote;

    voice.envelope.noteOn();
}

void SamplePlayback::stopVoice(int voiceIndex)
{
    auto& voice = voices[voiceIndex];

    if (voice.isActive && !voice.isReleasing)
    {
        voice.envelope.noteOff();
        voice.isReleasing = true;
    }
}

void SamplePlayback::stopAllVoices()
{
    for (auto& voice : voices)
    {
        voice.isActive = false;
        voice.isReleasing = false;
        voice.envelope.reset();
    }
}

float SamplePlayback::getSampleValue(int channel, float position)
{
    if (!hasSample() || channel >= numChannels)
        return 0.0f;

    return interpolateSample(channel, position);
}

float SamplePlayback::interpolateSample(int channel, float position)
{
    if (position < 0.0f || position >= sampleLength - 1)
        return 0.0f;

    int index1 = static_cast<int>(position);
    int index2 = index1 + 1;
    float fraction = position - index1;

    if (index2 >= sampleLength)
        return sampleBuffer.getSample(channel, index1);

    float sample1 = sampleBuffer.getSample(channel, index1);
    float sample2 = sampleBuffer.getSample(channel, index2);

    // Linear interpolation
    return sample1 + fraction * (sample2 - sample1);
}

float SamplePlayback::noteToFrequency(int midiNote)
{
    return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
}

float SamplePlayback::frequencyToPitchRatio(float targetFreq, float baseFreq)
{
    return targetFreq / baseFreq;
}
#include "MetalImpact.h"

MetalImpact::MetalImpact()
{
    // Initialize envelope parameters for sharp metal impacts
    envelopeParams.attack = 0.001f; // Very fast attack for impact
    envelopeParams.decay = 0.05f;
    envelopeParams.sustain = 0.3f;
    envelopeParams.release = 1.5f; // Long release for metal ring-out
    impactEnvelope.setParameters(envelopeParams);
}

MetalImpact::~MetalImpact()
{
}

void MetalImpact::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Prepare filters for metallic character
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;

    // Resonant filters for metallic ring
    resonantFilter1.prepare(spec);
    resonantFilter2.prepare(spec);
    updateFilterFrequencies();

    // High pass filter to emphasize metallic transients
    highPassFilter.prepare(spec);
    *highPassFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 150.0f, 0.7f);

    // Prepare envelope
    impactEnvelope.setSampleRate(sampleRate);

    // Initialize smoothers
    gainSmoother.reset(sampleRate, 0.01); // 10ms smoothing
    resonanceSmoother.reset(sampleRate, 0.05); // 50ms smoothing
    decaySmoother.reset(sampleRate, 0.1); // 100ms smoothing

    // Reset all oscillators
    for (auto& osc : resonantOscillators)
    {
        osc.reset();
    }
}

void MetalImpact::reset()
{
    resonantFilter1.reset();
    resonantFilter2.reset();
    highPassFilter.reset();
    impactEnvelope.reset();
    isActive = false;
    impactCounter = 0;
    lastImpactTime = 0;

    for (auto& osc : resonantOscillators)
    {
        osc.reset();
    }
}

void MetalImpact::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
    juce::AudioProcessorValueTreeState& apvts)
{
    // Get parameters
    bool enabled = *apvts.getRawParameterValue("METAL_ENABLE");
    if (!enabled) return;

    float gain = *apvts.getRawParameterValue("METAL_GAIN");
    float resonance = *apvts.getRawParameterValue("METAL_RESONANCE");
    float decay = *apvts.getRawParameterValue("METAL_DECAY");

    // Update smoothed parameters
    gainSmoother.setTargetValue(gain);
    resonanceSmoother.setTargetValue(resonance);
    decaySmoother.setTargetValue(decay);

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
    }

    // Update current parameters
    currentResonance = resonanceSmoother.getCurrentValue();
    currentDecay = decaySmoother.getCurrentValue();

    // Update filter frequencies based on resonance parameter
    updateFilterFrequencies();

    // Generate audio
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float currentGain = gainSmoother.getNextValue();

        // Generate impact components
        float transient = generateImpactTransient();
        float resonance = generateMetallicResonance();

        // Combine components
        float metalSound = transient * 0.7f + resonance * 0.8f;

        // Apply envelope and gain
        float envelopeValue = impactEnvelope.getNextSample();
        metalSound *= envelopeValue * currentGain;

        // Apply to all channels
        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.addSample(channel, sample, metalSound);
        }
    }

    // Apply filters for metallic character
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    highPassFilter.process(context);
    resonantFilter1.process(context);
    resonantFilter2.process(context);
}

void MetalImpact::processMidiNote(int midiNote, bool isNoteOn, float velocity)
{
    // Map MIDI notes to metal impacts
    // E3 (64) and above trigger metal impacts
    if (midiNote >= 64 && isNoteOn)
    {
        // Prevent too rapid impacts
        juce::uint32 currentTime = juce::Time::getMillisecondCounter();
        if (currentTime - lastImpactTime >= minImpactInterval)
        {
            triggerImpact(velocity, midiNote);
            lastImpactTime = currentTime;
        }
    }
}

void MetalImpact::triggerImpact(float velocity, int noteNumber)
{
    impactEnvelope.noteOn();
    isActive = true;
    impactCounter++;

    // Calculate base frequency from note
    float baseFreq = getFrequencyForNote(noteNumber);

    // Trigger resonant oscillators with metallic harmonics
    float harmonicRatios[] = { 1.0f, 2.1f, 3.3f, 4.7f, 6.2f, 8.1f, 10.3f, 12.8f };

    for (size_t i = 0; i < resonantOscillators.size(); ++i)
    {
        float freq = baseFreq * harmonicRatios[i];
        float amp = velocity * (1.0f / (i + 1)) * (0.5f + currentResonance * 0.5f);
        float decayTime = currentDecay * (2.0f - i * 0.1f); // Higher harmonics decay faster

        if (freq < 8000.0f) // Only trigger frequencies within reasonable range
        {
            resonantOscillators[i].trigger(freq, amp, decayTime);
        }
    }
}

float MetalImpact::generateImpactTransient()
{
    // Generate sharp transient for initial impact
    if (impactEnvelope.isActive())
    {
        // Use filtered noise for transient character
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Shape noise with quick decay
        float transientDecay = std::exp(-impactCounter * 0.001f);

        return noise * transientDecay * 0.3f;
    }

    return 0.0f;
}

float MetalImpact::generateMetallicResonance()
{
    float totalResonance = 0.0f;

    // Sum all active resonant oscillators
    for (auto& osc : resonantOscillators)
    {
        if (osc.active)
        {
            totalResonance += osc.getNextSample(currentSampleRate);
        }
    }

    return totalResonance;
}

float MetalImpact::getFrequencyForNote(int noteNumber)
{
    // Convert MIDI note to frequency (A4 = 440 Hz)
    return 440.0f * std::pow(2.0f, (noteNumber - 69) / 12.0f);
}

void MetalImpact::updateFilterFrequencies()
{
    // Update resonant filter frequencies based on resonance parameter
    float freq1 = 800.0f + currentResonance * 1200.0f; // 800-2000 Hz
    float freq2 = 2000.0f + currentResonance * 2000.0f; // 2000-4000 Hz
    float q = 1.0f + currentResonance * 4.0f; // Q from 1 to 5

    *resonantFilter1.coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        currentSampleRate, freq1, q, 1.5f);
    *resonantFilter2.coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        currentSampleRate, freq2, q * 0.7f, 1.3f);
}

// ImpactOscillator implementation
void MetalImpact::ImpactOscillator::trigger(float freq, float amp, float decayTime)
{
    frequency = freq;
    amplitude = amp;
    decay = 1.0f / (decayTime * 44100.0f); // Convert to per-sample decay
    phase = 0.0f;
    active = true;
}

float MetalImpact::ImpactOscillator::getNextSample(double sampleRate)
{
    if (!active || amplitude < 0.001f)
    {
        active = false;
        return 0.0f;
    }

    // Generate sine wave
    float sample = std::sin(phase) * amplitude;

    // Update phase
    phase += frequency * 2.0f * juce::MathConstants<float>::pi / sampleRate;
    if (phase >= 2.0f * juce::MathConstants<float>::pi)
        phase -= 2.0f * juce::MathConstants<float>::pi;

    // Apply decay
    amplitude *= (1.0f - decay);

    return sample;
}

void MetalImpact::ImpactOscillator::reset()
{
    phase = 0.0f;
    amplitude = 0.0f;
    active = false;
}
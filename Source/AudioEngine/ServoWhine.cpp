#include "ServoWhine.h"

ServoWhine::ServoWhine()
{
    // Initialize envelope parameters for servo activation
    envelopeParams.attack = 0.05f;
    envelopeParams.decay = 0.1f;
    envelopeParams.sustain = 0.9f;
    envelopeParams.release = 0.3f;
    servoEnvelope.setParameters(envelopeParams);
}

ServoWhine::~ServoWhine()
{
}

void ServoWhine::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Prepare filters for servo sound shaping
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;

    // Resonant filter for servo whine character
    resonantFilter.prepare(spec);
    *resonantFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 1200.0f, 3.0f, 1.5f);

    // High pass filter to clean up low end
    highPassFilter.prepare(spec);
    *highPassFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 200.0f, 0.7f);

    // Prepare envelope
    servoEnvelope.setSampleRate(sampleRate);

    // Initialize smoothers
    gainSmoother.reset(sampleRate, 0.02); // 20ms smoothing
    speedSmoother.reset(sampleRate, 0.05); // 50ms smoothing
    whineSmoother.reset(sampleRate, 0.03); // 30ms smoothing
    speedRamp.reset(sampleRate, 0.2); // 200ms speed ramping for realistic servo behavior
}

void ServoWhine::reset()
{
    resonantFilter.reset();
    highPassFilter.reset();
    servoEnvelope.reset();
    servoPhase = 0.0f;
    motorPhase = 0.0f;
    whinePhase = 0.0f;
    isActive = false;
    currentSpeed = 0.0f;
    targetSpeed = 0.0f;
}

void ServoWhine::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
    juce::AudioProcessorValueTreeState& apvts)
{
    // Get parameters
    bool enabled = *apvts.getRawParameterValue("SERVO_ENABLE");
    if (!enabled) return;

    float gain = *apvts.getRawParameterValue("SERVO_GAIN");
    float speed = *apvts.getRawParameterValue("SERVO_SPEED");
    float whine = *apvts.getRawParameterValue("SERVO_WHINE");

    // Update smoothed parameters
    gainSmoother.setTargetValue(gain);
    speedSmoother.setTargetValue(speed);
    whineSmoother.setTargetValue(whine);

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

    // Generate audio
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Update smoothed values
        float currentGain = gainSmoother.getNextValue();
        currentSpeed = speedSmoother.getNextValue();
        currentWhine = whineSmoother.getNextValue();

        // Apply speed ramping for realistic servo behavior
        float rampedSpeed = speedRamp.getNextValue();

        // Generate servo sounds
        float whineSound = generateServoWhine();
        float motorSound = generateMotorNoise();
        float gearSound = generateGearResonance();

        // Combine components based on parameters
        float servoSound = whineSound * currentWhine +
            motorSound * (1.0f - currentWhine * 0.5f) +
            gearSound * 0.3f;

        // Apply envelope and gain
        float envelopeValue = servoEnvelope.getNextSample();
        servoSound *= envelopeValue * currentGain;

        // Apply speed-based amplitude modulation
        float speedMod = 0.5f + (rampedSpeed / 100.0f) * 0.5f;
        servoSound *= speedMod;

        // Apply to all channels
        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.addSample(channel, sample, servoSound);
        }
    }

    // Apply filters
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    highPassFilter.process(context);
    resonantFilter.process(context);
}

void ServoWhine::processMidiNote(int midiNote, bool isNoteOn, float velocity)
{
    // Map MIDI notes to servo activation
    // D3 (62) and above trigger servo motors
    if (midiNote >= 62)
    {
        if (isNoteOn)
        {
            servoEnvelope.noteOn();
            isActive = true;

            // Map note to target speed (higher notes = higher speed)
            float noteSpeed = (midiNote - 62) * 5.0f + 10.0f; // 10-100+ range
            speedRamp.setTargetValue(juce::jmin(noteSpeed, 100.0f));
        }
        else
        {
            servoEnvelope.noteOff();
            speedRamp.setTargetValue(0.0f);
        }
    }
}

void ServoWhine::processMidiCC(int ccNumber, float ccValue)
{
    switch (ccNumber)
    {
    case 1: // Modulation wheel - controls servo speed
        speedRamp.setTargetValue(ccValue * 100.0f);
        break;
    case 11: // Expression - controls whine amount
        whineSmoother.setTargetValue(ccValue);
        break;
    default:
        break;
    }
}

float ServoWhine::generateServoWhine()
{
    // Generate characteristic servo whine (high-frequency oscillation)
    float baseFreq = 800.0f + (speedRamp.getCurrentValue() / 100.0f) * 2000.0f; // 800-2800 Hz

    whinePhase += baseFreq * 2.0f * juce::MathConstants<float>::pi / currentSampleRate;
    if (whinePhase >= 2.0f * juce::MathConstants<float>::pi)
        whinePhase -= 2.0f * juce::MathConstants<float>::pi;

    float whine = std::sin(whinePhase);

    // Add frequency modulation for more realistic whine
    float modFreq = 5.0f + (speedRamp.getCurrentValue() / 100.0f) * 15.0f;
    float modPhase = fmod(whinePhase * modFreq / baseFreq, 2.0f * juce::MathConstants<float>::pi);
    float modulation = std::sin(modPhase) * 0.1f + 1.0f;

    return whine * modulation * 0.4f;
}

float ServoWhine::generateMotorNoise()
{
    // Generate motor electrical noise
    motorPhase += (100.0f + speedRamp.getCurrentValue() * 5.0f) * 2.0f * juce::MathConstants<float>::pi / currentSampleRate;
    if (motorPhase >= 2.0f * juce::MathConstants<float>::pi)
        motorPhase -= 2.0f * juce::MathConstants<float>::pi;

    // Square wave for electrical switching noise
    float motorNoise = (std::sin(motorPhase) > 0.0f) ? 1.0f : -1.0f;

    // Add PWM-like modulation
    float pwmFreq = 20000.0f + speedRamp.getCurrentValue() * 100.0f;
    float pwmPhase = fmod(motorPhase * pwmFreq / (100.0f + speedRamp.getCurrentValue() * 5.0f),
        2.0f * juce::MathConstants<float>::pi);
    float pwmMod = (std::sin(pwmPhase) > 0.0f) ? 1.0f : 0.0f;

    return motorNoise * pwmMod * 0.15f;
}

float ServoWhine::generateGearResonance()
{
    // Generate gear resonance and mechanical noise
    float gearFreq = 60.0f + (speedRamp.getCurrentValue() / 100.0f) * 200.0f; // 60-260 Hz

    servoPhase += gearFreq * 2.0f * juce::MathConstants<float>::pi / currentSampleRate;
    if (servoPhase >= 2.0f * juce::MathConstants<float>::pi)
        servoPhase -= 2.0f * juce::MathConstants<float>::pi;

    // Generate mechanical resonance with harmonics
    float fundamental = std::sin(servoPhase);
    float harmonic2 = std::sin(servoPhase * 2.0f) * 0.3f;
    float harmonic3 = std::sin(servoPhase * 3.0f) * 0.15f;

    float gearSound = fundamental + harmonic2 + harmonic3;

    // Add mechanical irregularities
    float irregularityPhase = servoPhase * 0.1f;
    float irregularity = std::sin(irregularityPhase) * 0.2f + 1.0f;

    return gearSound * irregularity * 0.25f;
}
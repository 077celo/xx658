#include "HydraulicHiss.h"

HydraulicHiss::HydraulicHiss()
{
    // Initialize envelope parameters for hydraulic activation
    envelopeParams.attack = 0.1f;
    envelopeParams.decay = 0.2f;
    envelopeParams.sustain = 0.8f;
    envelopeParams.release = 0.5f;
    hydraulicEnvelope.setParameters(envelopeParams);
}

HydraulicHiss::~HydraulicHiss()
{
}

void HydraulicHiss::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Prepare filters for hydraulic sound shaping
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;

    // Low pass filter for main hiss (simulates air/fluid flow)
    lowPassFilter.prepare(spec);
    *lowPassFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 2000.0f, 0.7f);

    // High pass filter to remove low rumble
    highPassFilter.prepare(spec);
    *highPassFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 80.0f, 0.7f);

    // Band pass for pressure resonance
    bandPassFilter.prepare(spec);
    *bandPassFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 150.0f, 2.0f);

    // Prepare envelope
    hydraulicEnvelope.setSampleRate(sampleRate);

    // Initialize smoothers
    gainSmoother.reset(sampleRate, 0.02); // 20ms smoothing
    pressureSmoother.reset(sampleRate, 0.1); // 100ms smoothing
    flowSmoother.reset(sampleRate, 0.05); // 50ms smoothing
}

void HydraulicHiss::reset()
{
    lowPassFilter.reset();
    highPassFilter.reset();
    bandPassFilter.reset();
    hydraulicEnvelope.reset();
    pressurePhase = 0.0f;
    flowPhase = 0.0f;
    isActive = false;
}

void HydraulicHiss::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
    juce::AudioProcessorValueTreeState& apvts)
{
    // Get parameters
    bool enabled = *apvts.getRawParameterValue("HYDRAULIC_ENABLE");
    if (!enabled) return;

    float gain = *apvts.getRawParameterValue("HYDRAULIC_GAIN");
    float pressure = *apvts.getRawParameterValue("HYDRAULIC_PRESSURE");
    float flow = *apvts.getRawParameterValue("HYDRAULIC_FLOW");

    // Update smoothed parameters
    gainSmoother.setTargetValue(gain);
    pressureSmoother.setTargetValue(pressure);
    flowSmoother.setTargetValue(flow);

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

    // Generate audio
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Update smoothed values
        float currentGain = gainSmoother.getNextValue();
        currentPressure = pressureSmoother.getNextValue();
        currentFlow = flowSmoother.getNextValue();

        // Generate hydraulic sounds
        float hiss = generateHydraulicHiss();
        float pressureCycle = generatePressureCycle();
        float flowNoise = generateFlowNoise();

        // Combine components
        float hydraulicSound = hiss * 0.6f + pressureCycle * 0.3f + flowNoise * 0.4f;

        // Apply envelope
        float envelopeValue = hydraulicEnvelope.getNextSample();
        hydraulicSound *= envelopeValue * currentGain;

        // Apply to all channels
        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.addSample(channel, sample, hydraulicSound);
        }
    }

    // Apply filters
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    highPassFilter.process(context);
    lowPassFilter.process(context);

    // Apply band pass filter for pressure resonance
    juce::AudioBuffer<float> resonanceBuffer(buffer);
    juce::dsp::AudioBlock<float> resonanceBlock(resonanceBuffer);
    juce::dsp::ProcessContextReplacing<float> resonanceContext(resonanceBlock);
    bandPassFilter.process(resonanceContext);

    // Mix resonance back in
    for (int channel = 0; channel < numChannels; ++channel)
    {
        buffer.addFromWithRamp(channel, 0, resonanceBuffer.getReadPointer(channel),
            numSamples, 0.2f * currentPressure / 10.0f, 0.2f * currentPressure / 10.0f);
    }
}

void HydraulicHiss::processMidiNote(int midiNote, bool isNoteOn, float velocity)
{
    // Map MIDI notes to hydraulic activation
    // C3 (60) and above trigger hydraulic systems
    if (midiNote >= 60)
    {
        if (isNoteOn)
        {
            hydraulicEnvelope.noteOn();
            isActive = true;
        }
        else
        {
            hydraulicEnvelope.noteOff();
        }
    }
}

float HydraulicHiss::generateHydraulicHiss()
{
    // Generate white noise and shape it for hydraulic hiss
    float noise = random.nextFloat() * 2.0f - 1.0f;

    // Modulate noise intensity based on pressure
    float pressureModulation = 0.5f + (currentPressure / 10.0f) * 0.5f;
    noise *= pressureModulation;

    return noise * 0.3f;
}

float HydraulicHiss::generatePressureCycle()
{
    // Generate low-frequency pressure cycling
    float cycleFreq = 2.0f + (currentPressure - 1.0f) * 0.5f; // 2-6.5 Hz based on pressure
    pressurePhase += (cycleFreq * 2.0f * juce::MathConstants<float>::pi) / currentSampleRate;

    if (pressurePhase >= 2.0f * juce::MathConstants<float>::pi)
        pressurePhase -= 2.0f * juce::MathConstants<float>::pi;

    float cycle = std::sin(pressurePhase);

    // Add pressure buildup and release
    float buildupPhase = pressurePhase * 0.3f;
    float buildup = std::sin(buildupPhase) * 0.5f + 0.5f;

    return cycle * buildup * 0.4f;
}

float HydraulicHiss::generateFlowNoise()
{
    // Generate flow-based noise
    flowPhase += (20.0f + currentFlow * 30.0f) * 2.0f * juce::MathConstants<float>::pi / currentSampleRate;

    if (flowPhase >= 2.0f * juce::MathConstants<float>::pi)
        flowPhase -= 2.0f * juce::MathConstants<float>::pi;

    // Mix sine wave with noise for flow turbulence
    float flowTone = std::sin(flowPhase) * 0.3f;
    float flowTurbulence = (random.nextFloat() * 2.0f - 1.0f) * 0.2f;

    float flowModulation = currentFlow / 5.0f; // Normalize flow to 0-1

    return (flowTone + flowTurbulence) * flowModulation;
}
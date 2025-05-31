#include "GearGrind.h"

GearGrind::GearGrind()
{
    // Initialize envelope parameters for gear activation
    envelopeParams.attack = 0.2f;  // Slower gear engagement
    envelopeParams.decay = 0.3f;
    envelopeParams.sustain = 0.7f;
    envelopeParams.release = 0.8f;
    gearEnvelope.setParameters(envelopeParams);

    // Initialize gear teeth
    for (int i = 0; i < teethCount; ++i)
    {
        gearTeeth[i].position = static_cast<float>(i) / teethCount;
    }
}

GearGrind::~GearGrind()
{
}

void GearGrind::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Prepare filters for gear sound shaping
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;

    // Band pass filters for gear frequency ranges
    bandPassFilter1.prepare(spec);
    *bandPassFilter1.coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 400.0f, 2.0f);

    bandPassFilter2.prepare(spec);
    *bandPassFilter2.coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 800.0f, 1.5f);

    // High pass filter to remove low-end rumble
    highPassFilter.prepare(spec);
    *highPassFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 150.0f, 0.7f);

    // Notch filter to remove unwanted resonances
    notchFilter.prepare(spec);
    *notchFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate, 1200.0f, 3.0f);

    // Prepare envelope
    gearEnvelope.setSampleRate(sampleRate);

    // Initialize smoothers
    gainSmoother.reset(sampleRate, 0.02); // 20ms smoothing
    roughnessSmoother.reset(sampleRate, 0.1); // 100ms smoothing
    speedSmoother.reset(sampleRate, 0.2); // 200ms smoothing for gear speed changes
}

void GearGrind::reset()
{
    bandPassFilter1.reset();
    bandPassFilter2.reset();
    highPassFilter.reset();
    notchFilter.reset();
    gearEnvelope.reset();

    grindPhase1 = 0.0f;
    grindPhase2 = 0.0f;
    roughnessPhase = 0.0f;
    modulationPhase = 0.0f;
    isActive = false;

    // Reset gear teeth
    for (auto& tooth : gearTeeth)
    {
        tooth.engagement = 0.0f;
        tooth.isEngaged = false;
    }
}

void GearGrind::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
    juce::AudioProcessorValueTreeState& apvts)
{
    // Get parameters
    bool enabled = *apvts.getRawParameterValue("GEAR_ENABLE");
    if (!enabled) return;

    float gain = *apvts.getRawParameterValue("GEAR_GAIN");
    float roughness = *apvts.getRawParameterValue("GEAR_ROUGHNESS");
    float speed = *apvts.getRawParameterValue("GEAR_SPEED");

    // Update smoothed parameters
    gainSmoother.setTargetValue(gain);
    roughnessSmoother.setTargetValue(roughness);
    speedSmoother.setTargetValue(speed);

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
        currentRoughness = roughnessSmoother.getNextValue();
        currentSpeed = speedSmoother.getNextValue();

        // Update gear simulation
        updateGearTeeth();

        // Generate gear sounds
        float gearGrind = generateGearGrind();
        float gearMesh = generateGearMesh();
        float metalGrind = generateMetalGrind();
        float roughnessNoise = generateRoughnessNoise();

        // Combine components
        float gearSound = (gearGrind * 0.4f) + (gearMesh * 0.3f) +
            (metalGrind * 0.2f) + (roughnessNoise * currentRoughness * 0.3f);

        // Apply envelope
        float envelopeValue = gearEnvelope.getNextSample();
        gearSound *= envelopeValue * currentGain;

        // Apply to all channels
        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.addSample(channel, sample, gearSound);
        }
    }

    // Apply filters
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    highPassFilter.process(context);
    bandPassFilter1.process(context);
    bandPassFilter2.process(context);
    notchFilter.process(context);
}

void GearGrind::processMidiNote(int midiNote, bool isNoteOn, float velocity)
{
    // Map MIDI notes to gear activation
    // E3 (64) and above trigger gear systems
    if (midiNote >= 64)
    {
        if (isNoteOn)
        {
            gearEnvelope.noteOn();
            isActive = true;

            // Map note to gear speed
            float noteSpeed = (midiNote - 64) * 0.2f + 1.0f; // 1.0-10.0 range
            speedSmoother.setTargetValue(noteSpeed);
        }
        else
        {
            gearEnvelope.noteOff();
        }
    }
}

void GearGrind::processMidiCC(int ccNumber, float ccValue)
{
    // CC3 controls gear speed
    if (ccNumber == 3)
    {
        float speed = ccValue * 10.0f; // 0-10 range
        speedSmoother.setTargetValue(speed);
    }
    // CC4 controls roughness
    else if (ccNumber == 4)
    {
        roughnessSmoother.setTargetValue(ccValue * 2.0f); // 0-2 range
    }
}

float GearGrind::generateGearGrind()
{
    // Generate primary gear grinding frequency
    float grindFreq = 80.0f + currentSpeed * 40.0f; // 80-480 Hz
    grindPhase1 += (grindFreq * 2.0f * juce::MathConstants<float>::pi) / currentSampleRate;

    if (grindPhase1 >= 2.0f * juce::MathConstants<float>::pi)
        grindPhase1 -= 2.0f * juce::MathConstants<float>::pi;

    // Generate secondary grinding frequency (gear ratio simulation)
    float grindFreq2 = grindFreq * 1.33f; // Simulated gear ratio
    grindPhase2 += (grindFreq2 * 2.0f * juce::MathConstants<float>::pi) / currentSampleRate;

    if (grindPhase2 >= 2.0f * juce::MathConstants<float>::pi)
        grindPhase2 -= 2.0f * juce::MathConstants<float>::pi;

    // Create grinding sound with harmonics
    float grind1 = std::sin(grindPhase1) * 0.6f;
    float grind2 = std::sin(grindPhase2) * 0.4f;

    // Add square wave component for harsh grinding
    float square1 = (grindPhase1 < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;
    float square2 = (grindPhase2 < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;

    return (grind1 + grind2) * 0.7f + (square1 + square2) * 0.2f * currentRoughness;
}

float GearGrind::generateGearMesh()
{
    // Generate gear meshing frequency based on tooth engagement
    float engagementLevel = calculateGearEngagement();
    float meshFreq = currentSpeed * teethCount; // Teeth per second

    modulationPhase += (meshFreq * 2.0f * juce::MathConstants<float>::pi) / currentSampleRate;
    if (modulationPhase >= 2.0f * juce::MathConstants<float>::pi)
        modulationPhase -= 2.0f * juce::MathConstants<float>::pi;

    // Create impulse-like mesh sound
    float meshImpulse = std::sin(modulationPhase);

    // Add sharp transients for tooth engagement
    float transient = 0.0f;
    if (modulationPhase < 0.1f) // Sharp attack
    {
        transient = std::sin(modulationPhase * 31.4f) * (0.1f - modulationPhase) * 10.0f;
    }

    return (meshImpulse * 0.6f + transient * 0.4f) * engagementLevel;
}

float GearGrind::generateMetalGrind()
{
    // Generate metallic grinding component using filtered noise
    float metalNoise = random.nextFloat() * 2.0f - 1.0f;

    // Modulate noise amplitude with gear speed
    float speedMod = 0.3f + (currentSpeed / 10.0f) * 0.7f;
    metalNoise *= speedMod;

    // Add metallic resonance
    float resonanceFreq = 600.0f + currentSpeed * 50.0f;
    roughnessPhase += (resonanceFreq * 2.0f * juce::MathConstants<float>::pi) / currentSampleRate;

    if (roughnessPhase >= 2.0f * juce::MathConstants<float>::pi)
        roughnessPhase -= 2.0f * juce::MathConstants<float>::pi;

    float resonance = std::sin(roughnessPhase) * 0.3f;

    return metalNoise * 0.7f + resonance * 0.3f;
}

float GearGrind::generateRoughnessNoise()
{
    // Generate roughness-dependent noise
    float roughnessNoise = random.nextFloat() * 2.0f - 1.0f;

    // Filter noise based on roughness parameter
    static float roughnessFilter = 0.0f;
    float cutoff = 0.1f + currentRoughness * 0.4f; // Higher roughness = more high freq
    roughnessFilter += (roughnessNoise - roughnessFilter) * cutoff;

    return roughnessFilter * currentRoughness;
}

void GearGrind::updateGearTeeth()
{
    // Update gear tooth positions and engagement
    float speedRad = currentSpeed * 2.0f * juce::MathConstants<float>::pi / currentSampleRate;

    for (auto& tooth : gearTeeth)
    {
        tooth.position += speedRad / teethCount;
        if (tooth.position >= 1.0f)
            tooth.position -= 1.0f;

        // Calculate engagement based on position
        float engagementZone = 0.2f; // 20% of rotation is engagement zone
        if (tooth.position < engagementZone)
        {
            tooth.engagement = 1.0f - (tooth.position / engagementZone);
            tooth.isEngaged = true;
        }
        else
        {
            tooth.engagement = 0.0f;
            tooth.isEngaged = false;
        }
    }
}

float GearGrind::calculateGearEngagement()
{
    float totalEngagement = 0.0f;
    int engagedTeeth = 0;

    for (const auto& tooth : gearTeeth)
    {
        if (tooth.isEngaged)
        {
            totalEngagement += tooth.engagement;
            engagedTeeth++;
        }
    }

    return engagedTeeth > 0 ? totalEngagement / engagedTeeth : 0.0f;
}
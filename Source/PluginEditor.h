/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/ModuleTabs/ModuleTabs.h" // Assuming this path is correct for ModuleTabs

//==============================================================================
/**
*/
class GUNDAM_PluginAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::Timer // Inherit from juce::Timer
{
public:
    GUNDAM_PluginAudioProcessorEditor(GUNDAM_PluginAudioProcessor&);
    ~GUNDAM_PluginAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    // Implement the pure virtual timerCallback method from juce::Timer
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    GUNDAM_PluginAudioProcessor& audioProcessor;

    ModuleTabs moduleTabs; // This is a component itself, so it needs to be declared as a member

    // Declare Macro Sliders
    juce::Slider mechaSizeSlider;
    juce::Slider energySlider;
    juce::Slider intensitySlider;

    // Declare Macro Slider Labels
    juce::Label mechaSizeLabel;
    juce::Label energyLabel;
    juce::Label intensityLabel;

    // Declare Macro Slider Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mechaSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> energyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;

    // Declare Module Gain Sliders
    juce::Slider servoSlider;
    juce::Slider hissSlider;
    juce::Slider impactSlider;
    juce::Slider grindSlider;
    juce::Slider sampleSlider;

    // Declare Module Gain Slider Labels
    juce::Label servoLabel;
    juce::Label hissLabel;
    juce::Label impactLabel;
    juce::Label grindLabel;
    juce::Label sampleLabel;

    // Declare Module Gain Slider Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> servoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hissAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> impactAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grindAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sampleAttachment;

    // Meters and scope display
    float currentLeft = 0.0f;
    float currentRight = 0.0f;

    juce::AudioBuffer<float> scopeBuffer;
    juce::Image scopeImage; // Note: scopeImage is declared but not used in the provided paint() code.
    // juce::Timer scopeTimer; // Removed as the editor now inherits from juce::Timer

    // Activity indicators
    bool servoActive = false;
    bool impactActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GUNDAM_PluginAudioProcessorEditor)
};

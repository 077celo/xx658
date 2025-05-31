#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class GUNDAM_PluginAudioProcessor : public juce::AudioProcessorEditor,
    public juce::Slider::Listener,
    public juce::Button::Listener,
    public juce::ComboBox::Listener
{
public:
    GUNDAM_PluginAudioProcessor(MechaMovementAudioProcessor&);
    ~GUNDAM_PluginAudioProcessor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    // Component listeners
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;

private:
    // Reference to processor
    MechaMovementAudioProcessor& audioProcessor;

    // UI Components
    juce::GroupComponent hydraulicGroup, servoGroup, metalGroup, gearGroup, sampleGroup, masterGroup;

    // Hydraulic Hiss Controls
    juce::Slider hydraulicIntensitySlider, hydraulicFilterSlider, hydraulicGainSlider;
    juce::Label hydraulicIntensityLabel, hydraulicFilterLabel, hydraulicGainLabel;
    juce::ToggleButton hydraulicEnableButton;

    // Servo Whine Controls
    juce::Slider servoFreqSlider, servoModDepthSlider, servoGainSlider;
    juce::Label servoFreqLabel, servoModDepthLabel, servoGainLabel;
    juce::ToggleButton servoEnableButton;

    // Metal Impact Controls
    juce::Slider metalResonanceSlider, metalDecaySlider, metalGainSlider;
    juce::Label metalResonanceLabel, metalDecayLabel, metalGainLabel;
    juce::ToggleButton metalEnableButton;
    juce::Button metalTriggerButton;

    // Gear Grind Controls
    juce::Slider gearRoughnessSlider, gearSpeedSlider, gearGainSlider;
    juce::Label gearRoughnessLabel, gearSpeedLabel, gearGainLabel;
    juce::ToggleButton gearEnableButton;

    // Sample Playback Controls
    juce::Slider sampleGainSlider, samplePitchSlider;
    juce::Label sampleGainLabel, samplePitchLabel;
    juce::ToggleButton sampleEnableButton;
    juce::Button sampleTriggerButton;
    juce::ComboBox sampleSelectCombo;
    juce::Label sampleSelectLabel;

    // Master Controls
    juce::Slider masterGainSlider;
    juce::Label masterGainLabel;
    juce::ComboBox presetCombo;
    juce::Label presetLabel;
    juce::Button savePresetButton, loadPresetButton;

    // Macro Controls
    juce::Slider macro1Slider, macro2Slider, macro3Slider, macro4Slider;
    juce::Label macro1Label, macro2Label, macro3Label, macro4Label;

    // Parameter attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;

    // Helper methods
    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);
    void setupButton(juce::ToggleButton& button, const juce::String& buttonText);
    void setupComboBox(juce::ComboBox& combo, juce::Label& label, const juce::String& labelText);
    void createParameterAttachments();

    // Layout constants
    static constexpr int MARGIN = 10;
    static constexpr int GROUP_HEIGHT = 120;
    static constexpr int CONTROL_HEIGHT = 20;
    static constexpr int SLIDER_WIDTH = 80;
    static constexpr int BUTTON_HEIGHT = 25;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GUNDAM_PluginAudioProcessor)
};
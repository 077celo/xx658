#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GUNDAM_PluginAudioProcessor::GUNDAM_PluginAudioProcessor(MechaMovementAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set editor size
    setSize(1000, 700);

    // Setup UI groups
    addAndMakeVisible(hydraulicGroup);
    hydraulicGroup.setText("Hydraulic Hiss");
    hydraulicGroup.setTextLabelPosition(juce::Justification::centredTop);

    addAndMakeVisible(servoGroup);
    servoGroup.setText("Servo Whine");
    servoGroup.setTextLabelPosition(juce::Justification::centredTop);

    addAndMakeVisible(metalGroup);
    metalGroup.setText("Metal Impact");
    metalGroup.setTextLabelPosition(juce::Justification::centredTop);

    addAndMakeVisible(gearGroup);
    gearGroup.setText("Gear Grind");
    gearGroup.setTextLabelPosition(juce::Justification::centredTop);

    addAndMakeVisible(sampleGroup);
    sampleGroup.setText("Sample Playback");
    sampleGroup.setTextLabelPosition(juce::Justification::centredTop);

    addAndMakeVisible(masterGroup);
    masterGroup.setText("Master & Presets");
    masterGroup.setTextLabelPosition(juce::Justification::centredTop);

    // Setup Hydraulic controls
    setupSlider(hydraulicIntensitySlider, hydraulicIntensityLabel, "Intensity");
    setupSlider(hydraulicFilterSlider, hydraulicFilterLabel, "Filter");
    setupSlider(hydraulicGainSlider, hydraulicGainLabel, "Gain");
    setupButton(hydraulicEnableButton, "Enable");

    // Setup Servo controls
    setupSlider(servoFreqSlider, servoFreqLabel, "Frequency");
    setupSlider(servoModDepthSlider, servoModDepthLabel, "Mod Depth");
    setupSlider(servoGainSlider, servoGainLabel, "Gain");
    setupButton(servoEnableButton, "Enable");

    // Setup Metal controls
    setupSlider(metalResonanceSlider, metalResonanceLabel, "Resonance");
    setupSlider(metalDecaySlider, metalDecayLabel, "Decay");
    setupSlider(metalGainSlider, metalGainLabel, "Gain");
    setupButton(metalEnableButton, "Enable");
    addAndMakeVisible(metalTriggerButton);
    metalTriggerButton.setButtonText("Trigger");
    metalTriggerButton.addListener(this);

    // Setup Gear controls
    setupSlider(gearRoughnessSlider, gearRoughnessLabel, "Roughness");
    setupSlider(gearSpeedSlider, gearSpeedLabel, "Speed");
    setupSlider(gearGainSlider, gearGainLabel, "Gain");
    setupButton(gearEnableButton, "Enable");

    // Setup Sample controls
    setupSlider(sampleGainSlider, sampleGainLabel, "Gain");
    setupSlider(samplePitchSlider, samplePitchLabel, "Pitch");
    setupButton(sampleEnableButton, "Enable");
    addAndMakeVisible(sampleTriggerButton);
    sampleTriggerButton.setButtonText("Trigger");
    sampleTriggerButton.addListener(this);
    setupComboBox(sampleSelectCombo, sampleSelectLabel, "Sample");

    // Populate sample combo box
    sampleSelectCombo.addItem("Mecha Step 1", 1);
    sampleSelectCombo.addItem("Mecha Step 2", 2);
    sampleSelectCombo.addItem("Hydraulic Release", 3);
    sampleSelectCombo.addItem("Metal Clank", 4);
    sampleSelectCombo.addItem("Servo Motor", 5);
    sampleSelectCombo.setSelectedId(1);

    // Setup Master controls
    setupSlider(masterGainSlider, masterGainLabel, "Master Gain");
    setupComboBox(presetCombo, presetLabel, "Preset");

    // Populate preset combo
    presetCombo.addItem("Default", 1);
    presetCombo.addItem("Heavy Mech", 2);
    presetCombo.addItem("Light Scout", 3);
    presetCombo.addItem("Battle Damaged", 4);
    presetCombo.setSelectedId(1);

    addAndMakeVisible(savePresetButton);
    savePresetButton.setButtonText("Save");
    savePresetButton.addListener(this);

    addAndMakeVisible(loadPresetButton);
    loadPresetButton.setButtonText("Load");
    loadPresetButton.addListener(this);

    // Setup Macro controls
    setupSlider(macro1Slider, macro1Label, "Macro 1");
    setupSlider(macro2Slider, macro2Label, "Macro 2");
    setupSlider(macro3Slider, macro3Label, "Macro 3");
    setupSlider(macro4Slider, macro4Label, "Macro 4");

    // Create parameter attachments
    createParameterAttachments();
}

GUNDAM_PluginAudioProcessor::~GUNDAM_PluginAudioProcessor()
{
}

//==============================================================================
void GUNDAM_PluginAudioProcessor::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Mecha Movement Sound Generator", 0, 5, getWidth(), 30,
        juce::Justification::centred, 1);
}

void GUNDAM_PluginAudioProcessor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(40); // Title space

    // Top row - sound generators
    auto topRow = area.removeFromTop(GROUP_HEIGHT + 20);
    auto hydraulicArea = topRow.removeFromLeft(200);
    hydraulicGroup.setBounds(hydraulicArea);

    auto servoArea = topRow.removeFromLeft(200);
    servoGroup.setBounds(servoArea);

    auto metalArea = topRow.removeFromLeft(200);
    metalGroup.setBounds(metalArea);

    auto gearArea = topRow.removeFromLeft(200);
    gearGroup.setBounds(gearArea);

    auto sampleArea = topRow;
    sampleGroup.setBounds(sampleArea);

    // Layout hydraulic controls
    auto hArea = hydraulicArea.reduced(MARGIN);
    hArea.removeFromTop(20); // Group title space
    hydraulicEnableButton.setBounds(hArea.removeFromTop(BUTTON_HEIGHT));
    hArea.removeFromTop(5);

    auto hRow1 = hArea.removeFromTop(CONTROL_HEIGHT + 15);
    hydraulicIntensityLabel.setBounds(hRow1.removeFromTop(15));
    hydraulicIntensitySlider.setBounds(hRow1);

    auto hRow2 = hArea.removeFromTop(CONTROL_HEIGHT + 15);
    hydraulicFilterLabel.setBounds(hRow2.removeFromTop(15));
    hydraulicFilterSlider.setBounds(hRow2);

    auto hRow3 = hArea.removeFromTop(CONTROL_HEIGHT + 15);
    hydraulicGainLabel.setBounds(hRow3.removeFromTop(15));
    hydraulicGainSlider.setBounds(hRow3);

    // Layout servo controls
    auto sArea = servoArea.reduced(MARGIN);
    sArea.removeFromTop(20);
    servoEnableButton.setBounds(sArea.removeFromTop(BUTTON_HEIGHT));
    sArea.removeFromTop(5);

    auto sRow1 = sArea.removeFromTop(CONTROL_HEIGHT + 15);
    servoFreqLabel.setBounds(sRow1.removeFromTop(15));
    servoFreqSlider.setBounds(sRow1);

    auto sRow2 = sArea.removeFromTop(CONTROL_HEIGHT + 15);
    servoModDepthLabel.setBounds(sRow2.removeFromTop(15));
    servoModDepthSlider.setBounds(sRow2);

    auto sRow3 = sArea.removeFromTop(CONTROL_HEIGHT + 15);
    servoGainLabel.setBounds(sRow3.removeFromTop(15));
    servoGainSlider.setBounds(sRow3);

    // Layout metal controls
    auto mArea = metalArea.reduced(MARGIN);
    mArea.removeFromTop(20);
    metalEnableButton.setBounds(mArea.removeFromTop(BUTTON_HEIGHT));
    mArea.removeFromTop(5);
    metalTriggerButton.setBounds(mArea.removeFromTop(BUTTON_HEIGHT));
    mArea.removeFromTop(5);

    auto mRow1 = mArea.removeFromTop(CONTROL_HEIGHT + 15);
    metalResonanceLabel.setBounds(mRow1.removeFromTop(15));
    metalResonanceSlider.setBounds(mRow1);

    auto mRow2 = mArea.removeFromTop(CONTROL_HEIGHT + 15);
    metalDecayLabel.setBounds(mRow2.removeFromTop(15));
    metalDecaySlider.setBounds(mRow2);

    // Layout gear controls
    auto gArea = gearArea.reduced(MARGIN);
    gArea.removeFromTop(20);
    gearEnableButton.setBounds(gArea.removeFromTop(BUTTON_HEIGHT));
    gArea.removeFromTop(5);

    auto gRow1 = gArea.removeFromTop(CONTROL_HEIGHT + 15);
    gearRoughnessLabel.setBounds(gRow1.removeFromTop(15));
    gearRoughnessSlider.setBounds(gRow1);

    auto gRow2 = gArea.removeFromTop(CONTROL_HEIGHT + 15);
    gearSpeedLabel.setBounds(gRow2.removeFromTop(15));
    gearSpeedSlider.setBounds(gRow2);

    auto gRow3 = gArea.removeFromTop(CONTROL_HEIGHT + 15);
    gearGainLabel.setBounds(gRow3.removeFromTop(15));
    gearGainSlider.setBounds(gRow3);

    // Layout sample controls
    auto sampArea = sampleArea.reduced(MARGIN);
    sampArea.removeFromTop(20);
    sampleEnableButton.setBounds(sampArea.removeFromTop(BUTTON_HEIGHT));
    sampArea.removeFromTop(5);
    sampleTriggerButton.setBounds(sampArea.removeFromTop(BUTTON_HEIGHT));
    sampArea.removeFromTop(5);

    auto sampRow1 = sampArea.removeFromTop(CONTROL_HEIGHT + 15);
    sampleSelectLabel.setBounds(sampRow1.removeFromTop(15));
    sampleSelectCombo.setBounds(sampRow1);

    auto sampRow2 = sampArea.removeFromTop(CONTROL_HEIGHT + 15);
    sampleGainLabel.setBounds(sampRow2.removeFromTop(15));
    sampleGainSlider.setBounds(sampRow2);

    // Bottom row - master and macros
    area.removeFromTop(10);
    auto bottomRow = area.removeFromTop(GROUP_HEIGHT + 20);

    auto masterArea = bottomRow.removeFromLeft(300);
    masterGroup.setBounds(masterArea);

    // Layout master controls
    auto mastArea = masterArea.reduced(MARGIN);
    mastArea.removeFromTop(20);

    auto mastRow1 = mastArea.removeFromTop(CONTROL_HEIGHT + 15);
    masterGainLabel.setBounds(mastRow1.removeFromTop(15));
    masterGainSlider.setBounds(mastRow1);

    auto mastRow2 = mastArea.removeFromTop(CONTROL_HEIGHT + 15);
    presetLabel.setBounds(mastRow2.removeFromTop(15));
    presetCombo.setBounds(mastRow2);

    auto buttonRow = mastArea.removeFromTop(BUTTON_HEIGHT);
    savePresetButton.setBounds(buttonRow.removeFromLeft(70));
    buttonRow.removeFromLeft(10);
    loadPresetButton.setBounds(buttonRow.removeFromLeft(70));

    // Layout macro controls
    auto macroArea = bottomRow;
    auto macroRow1 = macroArea.removeFromTop((GROUP_HEIGHT + 20) / 2);
    auto macroRow2 = macroArea;

    auto macro1Area = macroRow1.removeFromLeft(macroRow1.getWidth() / 2);
    auto macro2Area = macroRow1;
    auto macro3Area = macroRow2.removeFromLeft(macroRow2.getWidth() / 2);
    auto macro4Area = macroRow2;

    // Macro 1
    macro1Label.setBounds(macro1Area.removeFromTop(15));
    macro1Slider.setBounds(macro1Area.reduced(10));

    // Macro 2
    macro2Label.setBounds(macro2Area.removeFromTop(15));
    macro2Slider.setBounds(macro2Area.reduced(10));

    // Macro 3
    macro3Label.setBounds(macro3Area.removeFromTop(15));
    macro3Slider.setBounds(macro3Area.reduced(10));

    // Macro 4
    macro4Label.setBounds(macro4Area.removeFromTop(15));
    macro4Slider.setBounds(macro4Area.reduced(10));
}

void GUNDAM_PluginAudioProcessor::sliderValueChanged(juce::Slider* slider)
{
    // Slider changes are handled by parameter attachments
}

void GUNDAM_PluginAudioProcessor::buttonClicked(juce::Button* button)
{
    if (button == &metalTriggerButton)
    {
        audioProcessor.triggerMetalImpact();
    }
    else if (button == &sampleTriggerButton)
    {
        audioProcessor.triggerSample();
    }
    else if (button == &savePresetButton)
    {
        // TODO: Implement preset saving
    }
    else if (button == &loadPresetButton)
    {
        // TODO: Implement preset loading
    }
}

void GUNDAM_PluginAudioProcessor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &sampleSelectCombo)
    {
        audioProcessor.setSampleIndex(sampleSelectCombo.getSelectedId() - 1);
    }
    else if (comboBox == &presetCombo)
    {
        // TODO: Implement preset changing
    }
}

void GUNDAM_PluginAudioProcessor::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    slider.addListener(this);

    addAndMakeVisible(label);
    label.setText(labelText, juce::dontSendNotification);
    label.attachToComponent(&slider, false);
}

void GUNDAM_PluginAudioProcessor::setupButton(juce::ToggleButton& button, const juce::String& buttonText)
{
    addAndMakeVisible(button);
    button.setButtonText(buttonText);
    button.addListener(this);
}

void GUNDAM_PluginAudioProcessor::setupComboBox(juce::ComboBox& combo, juce::Label& label, const juce::String& labelText)
{
    addAndMakeVisible(combo);
    combo.addListener(this);

    addAndMakeVisible(label);
    label.setText(labelText, juce::dontSendNotification);
    label.attachToComponent(&combo, false);
}

void GUNDAM_PluginAudioProcessor::createParameterAttachments()
{
    auto& params = audioProcessor.getParameters();

    // Create slider attachments
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "hydraulicIntensity", hydraulicIntensitySlider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "hydraulicFilter", hydraulicFilterSlider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "hydraulicGain", hydraulicGainSlider));

    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "servoFreq", servoFreqSlider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "servoModDepth", servoModDepthSlider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "servoGain", servoGainSlider));

    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "metalResonance", metalResonanceSlider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "metalDecay", metalDecaySlider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "metalGain", metalGainSlider));

    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "gearRoughness", gearRoughnessSlider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "gearSpeed", gearSpeedSlider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "gearGain", gearGainSlider));

    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "sampleGain", sampleGainSlider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "samplePitch", samplePitchSlider));

    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "masterGain", masterGainSlider));

    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "macro1", macro1Slider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "macro2", macro2Slider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "macro3", macro3Slider));
    sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "macro4", macro4Slider));

    // Create button attachments
    buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "hydraulicEnable", hydraulicEnableButton));
    buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "servoEnable", servoEnableButton));
    buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "metalEnable", metalEnableButton));
    buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "gearEnable", gearEnableButton));
    buttonAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "sampleEnable", sampleEnableButton));
}
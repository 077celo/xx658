/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GUNDAM_PluginAudioProcessorEditor::GUNDAM_PluginAudioProcessorEditor(GUNDAM_PluginAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(600, 400); // Increased size to accommodate new controls

    // Add moduleTabs
    addAndMakeVisible(moduleTabs);

    // --- Macro Controls ---
    // Mecha Size
    mechaSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mechaSizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(mechaSizeSlider);
    mechaSizeLabel.setText("Mecha Size", juce::dontSendNotification);
    mechaSizeLabel.attachToComponent(&mechaSizeSlider, false);
    addAndMakeVisible(mechaSizeLabel);
    mechaSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "macroMechaSize", mechaSizeSlider);

    // Energy
    energySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    energySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(energySlider);
    energyLabel.setText("Energy", juce::dontSendNotification);
    energyLabel.attachToComponent(&energySlider, false);
    addAndMakeVisible(energyLabel);
    energyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "macroEnergy", energySlider);

    // Intensity
    intensitySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    intensitySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(intensitySlider);
    intensityLabel.setText("Intensity", juce::dontSendNotification);
    intensityLabel.attachToComponent(&intensitySlider, false);
    addAndMakeVisible(intensityLabel);
    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "macroIntensity", intensitySlider);


    // --- Module Gain Controls ---
    servoSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    servoSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    addAndMakeVisible(servoSlider);
    servoLabel.setText("Servo Gain", juce::dontSendNotification);
    servoLabel.attachToComponent(&servoSlider, false);
    addAndMakeVisible(servoLabel);
    servoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "servoGain", servoSlider);

    hissSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    hissSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    addAndMakeVisible(hissSlider);
    hissLabel.setText("Hiss Gain", juce::dontSendNotification);
    hissLabel.attachToComponent(&hissSlider, false);
    addAndMakeVisible(hissLabel);
    hissAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "hissGain", hissSlider);

    impactSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    impactSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    addAndMakeVisible(impactSlider);
    impactLabel.setText("Impact Gain", juce::dontSendNotification);
    impactLabel.attachToComponent(&impactSlider, false);
    addAndMakeVisible(impactLabel);
    impactAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "impactGain", impactSlider);

    grindSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    grindSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    addAndMakeVisible(grindSlider);
    grindLabel.setText("Grind Gain", juce::dontSendNotification);
    grindLabel.attachToComponent(&grindSlider, false);
    addAndMakeVisible(grindLabel);
    grindAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "grindGain", grindSlider);

    sampleSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sampleSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    addAndMakeVisible(sampleSlider);
    sampleLabel.setText("Sample Gain", juce::dontSendNotification);
    sampleLabel.attachToComponent(&sampleSlider, false);
    addAndMakeVisible(sampleLabel);
    sampleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "sampleGain", sampleSlider);

    // Start the scope/meter updater:
    // The editor now inherits from juce::Timer, so call startTimerHz directly.
    // The logic previously in scopeTimer.onTimer is now in the timerCallback() method below.
    startTimerHz(30);
}


GUNDAM_PluginAudioProcessorEditor::~GUNDAM_PluginAudioProcessorEditor()
{
    // Stop the timer when the editor is destroyed
    stopTimer();
}

// Implement the pure virtual timerCallback method from juce::Timer
void GUNDAM_PluginAudioProcessorEditor::timerCallback()
{
    currentLeft = audioProcessor.peakLevelLeft.load();
    currentRight = audioProcessor.peakLevelRight.load();

    // Copy scope buffer safely
    const juce::ScopedLock sl(audioProcessor.scopeLock);
    scopeBuffer.makeCopyOf(audioProcessor.scopeBuffer);

    // You might also update LED states here if they are controlled by the processor
    // servoActive = audioProcessor.getServoActiveState(); // Example if you add a getter in processor
    // impactActive = audioProcessor.getImpactActiveState(); // Example

    repaint();
}


//==============================================================================
void GUNDAM_PluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // You can add a title or section labels here if moduleTabs doesn't handle it
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(18.0f));
    g.drawFittedText("MACRO CONTROLS", getLocalBounds().removeFromTop(30).reduced(10, 0), juce::Justification::centredLeft, 1);

    // Draw "MODULE GAIN CONTROLS" text. This should be in paint(), not resized().
    // Calculate where this text should appear based on the layout in resized().
    // For simplicity here, let's assume it's below the macro sliders.
    auto boundsForModuleGainText = getLocalBounds();
    boundsForModuleGainText.removeFromTop(30); // Macro Controls title
    boundsForModuleGainText.removeFromTop(120); // Macro sliders
    boundsForModuleGainText.removeFromTop(20); // Padding

    g.drawFittedText("MODULE GAIN CONTROLS", boundsForModuleGainText.removeFromTop(30).reduced(10, 0), juce::Justification::centredLeft, 1);

    // Add this block at the end of your paint method:
    auto bounds = getLocalBounds();
    auto meterHeight = 20;
    auto scopeHeight = 80;

    // Adjust bounds to account for existing UI elements if necessary,
    // or ensure these new elements are placed at the bottom.
    // For now, assuming they should be at the very bottom of the editor.
    bounds.removeFromTop(getLocalBounds().getHeight() - meterHeight - scopeHeight); // Remove everything above the new elements

    juce::Rectangle<int> meterArea = bounds.removeFromBottom(meterHeight);
    juce::Rectangle<int> scopeArea = bounds.removeFromBottom(scopeHeight);

    // Draw meters
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(meterArea);
    int meterWidth = meterArea.getWidth() / 2;
    juce::Rectangle<int> leftMeter = meterArea.removeFromLeft(meterWidth);
    juce::Rectangle<int> rightMeter = meterArea;

    g.setColour(juce::Colours::limegreen);
    g.fillRect(leftMeter.removeFromBottom((int)(currentLeft * leftMeter.getHeight())));
    g.fillRect(rightMeter.removeFromBottom((int)(currentRight * rightMeter.getHeight())));

    // Draw scope waveform (left channel only)
    g.setColour(juce::Colours::black);
    g.fillRect(scopeArea);
    g.setColour(juce::Colours::cyan);
    if (scopeBuffer.getNumSamples() > 0 && scopeBuffer.getNumChannels() > 0)
    {
        auto* data = scopeBuffer.getReadPointer(0);
        int width = scopeArea.getWidth();
        int height = scopeArea.getHeight();
        juce::Path waveform;
        waveform.startNewSubPath(scopeArea.getX(), scopeArea.getCentreY());

        for (int x = 0; x < width; ++x)
        {
            int index = juce::jmap(x, 0, width, 0, scopeBuffer.getNumSamples());
            float sample = data[index];
            float y = juce::jmap(sample, -1.0f, 1.0f, (float)scopeArea.getBottom(), (float)scopeArea.getY());
            waveform.lineTo(scopeArea.getX() + x, y);
        }

        g.strokePath(waveform, juce::PathStrokeType(1.5f));
    }

    // Draw module indicators (LED-style)
    int ledSize = 12;
    g.setColour(servoActive ? juce::Colours::green : juce::Colours::darkgreen);
    g.fillEllipse(10, 10, ledSize, ledSize);
    g.setColour(juce::Colours::white);
    g.drawFittedText("Servo", 26, 10, 50, ledSize, juce::Justification::left, 1);

    g.setColour(impactActive ? juce::Colours::red : juce::Colours::darkred);
    g.fillEllipse(100, 10, ledSize, ledSize);
    g.setColour(juce::Colours::white);
    g.drawFittedText("Impact", 116, 10, 60, ledSize, juce::Justification::left, 1);
}


void GUNDAM_PluginAudioProcessorEditor::resized()
{
    // This is where you'll lay out the positions of your subcomponents.
    auto bounds = getLocalBounds();


    // Reserve space for the MACRO CONTROLS title
    auto topTitleArea = bounds.removeFromTop(30);

    // Layout for Macro Sliders (top row)
    auto macroSliderArea = bounds.removeFromTop(120).reduced(10); // 120 height for slider + label
    int macroSliderWidth = macroSliderArea.getWidth() / 3;


    mechaSizeLabel.setBounds(macroSliderArea.removeFromLeft(macroSliderWidth).reduced(0, 20)); // Adjust label position
    mechaSizeSlider.setBounds(mechaSizeLabel.getBounds().translated(0, mechaSizeLabel.getHeight()));


    energyLabel.setBounds(macroSliderArea.removeFromLeft(macroSliderWidth).reduced(0, 20));
    energySlider.setBounds(energyLabel.getBounds().translated(0, energyLabel.getHeight()));


    intensityLabel.setBounds(macroSliderArea.reduced(0, 20)); // Last one takes remaining width
    intensitySlider.setBounds(intensityLabel.getBounds().translated(0, intensityLabel.getHeight()));

    // Reserve space for the MODULE GAIN CONTROLS title (this space calculation is correct for layout)
    bounds.removeFromTop(20); // Some padding
    bounds.removeFromTop(30); // Space for the text itself


    // Layout for Module Gain Sliders (below macros)
    auto moduleGainSliderArea = bounds.reduced(10); // Use remaining bounds
    int sliderWidth = 70;
    int sliderHeight = 80; // Enough for knob and textbox
    int labelHeight = 20; // For the label above the slider
    int padding = 10;


    int yPos = moduleGainSliderArea.getY() + labelHeight; // Start Y position for this row


    // Servo
    servoLabel.setBounds(moduleGainSliderArea.getX() + padding, yPos - labelHeight, sliderWidth, labelHeight);
    servoSlider.setBounds(moduleGainSliderArea.getX() + padding, yPos, sliderWidth, sliderHeight);


    // Hiss
    hissLabel.setBounds(moduleGainSliderArea.getX() + padding + sliderWidth + padding, yPos - labelHeight, sliderWidth, labelHeight);
    hissSlider.setBounds(moduleGainSliderArea.getX() + padding + sliderWidth + padding, yPos, sliderWidth, sliderHeight);

    // Impact
    impactLabel.setBounds(moduleGainSliderArea.getX() + padding + (sliderWidth + padding) * 2, yPos - labelHeight, sliderWidth, labelHeight);
    impactSlider.setBounds(moduleGainSliderArea.getX() + padding + (sliderWidth + padding) * 2, yPos, sliderWidth, sliderHeight);

    // Grind
    grindLabel.setBounds(moduleGainSliderArea.getX() + padding + (sliderWidth + padding) * 3, yPos - labelHeight, sliderWidth, labelHeight);
    grindSlider.setBounds(moduleGainSliderArea.getX() + padding + (sliderWidth + padding) * 3, yPos, sliderWidth, sliderHeight);

    // Sample
    sampleLabel.setBounds(moduleGainSliderArea.getX() + padding + (sliderWidth + padding) * 4, yPos - labelHeight, sliderWidth, labelHeight);
    sampleSlider.setBounds(moduleGainSliderArea.getX() + padding + (sliderWidth + padding) * 4, yPos, sliderWidth, sliderHeight);

    // This makes moduleTabs fill the remaining space.
    moduleTabs.setBounds(bounds);
}

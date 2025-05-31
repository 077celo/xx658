#include "ModuleTabs.h"

ModuleTabs::ModuleTabs()
{
    tabs.addTab("Servo", juce::Colours::lightblue, &servoPanel, false);
    tabs.addTab("Hiss", juce::Colours::lightgreen, &hissPanel, false);
    tabs.addTab("Impact", juce::Colours::lightcoral, &impactPanel, false);
    tabs.addTab("Grind", juce::Colours::lightyellow, &grindPanel, false);
    tabs.addTab("Sample", juce::Colours::lightgrey, &samplePanel, false);

    addAndMakeVisible(tabs);
}

void ModuleTabs::resized()
{
    tabs.setBounds(getLocalBounds());
}

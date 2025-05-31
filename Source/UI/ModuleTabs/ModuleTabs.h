#pragma once

#include <JuceHeader.h>

class ModuleTabs : public juce::Component
{
public:
    ModuleTabs();
    ~ModuleTabs() override = default;

    void resized() override;

private:
    juce::TabbedComponent tabs{ juce::TabbedButtonBar::TabsAtTop };

    // Temporary placeholders (can be replaced with real panels later)
    juce::Component servoPanel;
    juce::Component hissPanel;
    juce::Component impactPanel;
    juce::Component grindPanel;
    juce::Component samplePanel;
};

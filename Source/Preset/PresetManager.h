#pragma once

#include <JuceHeader.h>

class PresetManager
{
public:
    PresetManager(juce::AudioProcessorValueTreeState& vts);
    ~PresetManager();

    // Preset operations
    void savePreset(const juce::String& presetName);
    void loadPreset(const juce::String& presetName);
    void deletePreset(const juce::String& presetName);

    // Factory presets
    void loadFactoryPreset(int presetIndex);
    void createFactoryPresets();

    // Preset management
    juce::StringArray getPresetNames() const;
    juce::File getPresetsDirectory() const;

    // Current preset info
    juce::String getCurrentPresetName() const { return currentPresetName; }
    bool isCurrentPresetModified() const { return isModified; }
    void setCurrentPresetModified(bool modified) { isModified = modified; }

private:
    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::String currentPresetName;
    bool isModified;

    // Factory preset definitions
    struct FactoryPreset
    {
        juce::String name;
        juce::var presetData;
    };

    std::vector<FactoryPreset> factoryPresets;

    // Helper methods
    juce::File getPresetFile(const juce::String& presetName) const;
    juce::var getStateAsVar() const;
    void setStateFromVar(const juce::var& state);
    void initializeFactoryPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
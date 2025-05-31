#include "PresetManager.h"

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& vts)
    : valueTreeState(vts), currentPresetName("Default"), isModified(false)
{
    initializeFactoryPresets();
}

PresetManager::~PresetManager()
{
}

void PresetManager::savePreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return;

    auto presetFile = getPresetFile(presetName);
    auto presetDir = presetFile.getParentDirectory();

    if (!presetDir.exists())
        presetDir.createDirectory();

    auto state = getStateAsVar();

    juce::DynamicObject::Ptr presetObject = new juce::DynamicObject();
    presetObject->setProperty("name", presetName);
    presetObject->setProperty("version", "1.0");
    presetObject->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(false));
    presetObject->setProperty("state", state);

    juce::var presetVar(presetObject.get());
    juce::String jsonString = juce::JSON::toString(presetVar);

    if (presetFile.replaceWithText(jsonString))
    {
        currentPresetName = presetName;
        isModified = false;
        DBG("Preset saved: " + presetName);
    }
    else
    {
        DBG("Failed to save preset: " + presetName);
    }
}

void PresetManager::loadPreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return;

    auto presetFile = getPresetFile(presetName);

    if (!presetFile.exists())
    {
        DBG("Preset file not found: " + presetName);
        return;
    }

    auto jsonString = presetFile.loadFileAsString();
    auto presetVar = juce::JSON::parse(jsonString);

    if (presetVar.isObject())
    {
        auto* presetObject = presetVar.getDynamicObject();
        if (presetObject != nullptr && presetObject->hasProperty("state"))
        {
            auto state = presetObject->getProperty("state");
            setStateFromVar(state);
            currentPresetName = presetName;
            isModified = false;
            DBG("Preset loaded: " + presetName);
        }
    }
    else
    {
        DBG("Invalid preset format: " + presetName);
    }
}

void PresetManager::deletePreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return;

    auto presetFile = getPresetFile(presetName);

    if (presetFile.exists())
    {
        if (presetFile.deleteFile())
        {
            DBG("Preset deleted: " + presetName);
        }
        else
        {
            DBG("Failed to delete preset: " + presetName);
        }
    }
}

void PresetManager::loadFactoryPreset(int presetIndex)
{
    if (presetIndex >= 0 && presetIndex < static_cast<int>(factoryPresets.size()))
    {
        const auto& preset = factoryPresets[presetIndex];
        setStateFromVar(preset.presetData);
        currentPresetName = preset.name;
        isModified = false;
        DBG("Factory preset loaded: " + preset.name);
    }
}

void PresetManager::createFactoryPresets()
{
    // This method can be called to save current factory presets to disk
    for (const auto& preset : factoryPresets)
    {
        auto presetFile = getPresetFile(preset.name);
        auto presetDir = presetFile.getParentDirectory();

        if (!presetDir.exists())
            presetDir.createDirectory();

        juce::DynamicObject::Ptr presetObject = new juce::DynamicObject();
        presetObject->setProperty("name", preset.name);
        presetObject->setProperty("version", "1.0");
        presetObject->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(false));
        presetObject->setProperty("state", preset.presetData);
        presetObject->setProperty("factory", true);

        juce::var presetVar(presetObject.get());
        juce::String jsonString = juce::JSON::toString(presetVar);

        presetFile.replaceWithText(jsonString);
    }
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray presetNames;

    // Add factory presets
    for (const auto& preset : factoryPresets)
    {
        presetNames.add(preset.name);
    }

    // Add user presets
    auto presetsDir = getPresetsDirectory();
    if (presetsDir.exists())
    {
        auto presetFiles = presetsDir.findChildFiles(juce::File::findFiles, false, "*.preset");
        for (const auto& file : presetFiles)
        {
            auto name = file.getFileNameWithoutExtension();
            if (!presetNames.contains(name))
                presetNames.add(name);
        }
    }

    return presetNames;
}

juce::File PresetManager::getPresetsDirectory() const
{
    auto userAppData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return userAppData.getChildFile("MechaMovementSoundGenerator").getChildFile("Presets");
}

juce::File PresetManager::getPresetFile(const juce::String& presetName) const
{
    return getPresetsDirectory().getChildFile(presetName + ".preset");
}

juce::var PresetManager::getStateAsVar() const
{
    auto state = valueTreeState.copyState();
    return juce::var(state.createXml().get());
}

void PresetManager::setStateFromVar(const juce::var& state)
{
    if (state.isString())
    {
        auto xmlState = juce::XmlDocument::parse(state.toString());
        if (xmlState != nullptr)
        {
            auto newState = juce::ValueTree::fromXml(*xmlState);
            if (newState.isValid())
            {
                valueTreeState.replaceState(newState);
            }
        }
    }
}

void PresetManager::initializeFactoryPresets()
{
    factoryPresets.clear();

    // Default preset
    {
        juce::DynamicObject::Ptr defaultState = new juce::DynamicObject();
        defaultState->setProperty("hydraulicIntensity", 0.5f);
        defaultState->setProperty("hydraulicFilter", 0.7f);
        defaultState->setProperty("hydraulicGain", 0.6f);
        defaultState->setProperty("hydraulicEnable", true);

        defaultState->setProperty("servoFreq", 0.4f);
        defaultState->setProperty("servoModDepth", 0.3f);
        defaultState->setProperty("servoGain", 0.5f);
        defaultState->setProperty("servoEnable", true);

        defaultState->setProperty("metalResonance", 0.6f);
        defaultState->setProperty("metalDecay", 0.5f);
        defaultState->setProperty("metalGain", 0.7f);
        defaultState->setProperty("metalEnable", true);

        defaultState->setProperty("gearRoughness", 0.4f);
        defaultState->setProperty("gearSpeed", 0.5f);
        defaultState->setProperty("gearGain", 0.5f);
        defaultState->setProperty("gearEnable", true);

        defaultState->setProperty("sampleGain", 0.6f);
        defaultState->setProperty("samplePitch", 0.5f);
        defaultState->setProperty("sampleEnable", true);

        defaultState->setProperty("masterGain", 0.7f);

        defaultState->setProperty("macro1", 0.0f);
        defaultState->setProperty("macro2", 0.0f);
        defaultState->setProperty("macro3", 0.0f);
        defaultState->setProperty("macro4", 0.0f);

        factoryPresets.push_back({ "Default", juce::var(defaultState.get()) });
    }

    // Heavy Mech preset
    {
        juce::DynamicObject::Ptr heavyState = new juce::DynamicObject();
        heavyState->setProperty("hydraulicIntensity", 0.8f);
        heavyState->setProperty("hydraulicFilter", 0.3f);
        heavyState->setProperty("hydraulicGain", 0.9f);
        heavyState->setProperty("hydraulicEnable", true);

        heavyState->setProperty("servoFreq", 0.2f);
        heavyState->setProperty("servoModDepth", 0.6f);
        heavyState->setProperty("servoGain", 0.8f);
        heavyState->setProperty("servoEnable", true);

        heavyState->setProperty("metalResonance", 0.9f);
        heavyState->setProperty("metalDecay", 0.8f);
        heavyState->setProperty("metalGain", 1.0f);
        heavyState->setProperty("metalEnable", true);

        heavyState->setProperty("gearRoughness", 0.8f);
        heavyState->setProperty("gearSpeed", 0.3f);
        heavyState->setProperty("gearGain", 0.9f);
        heavyState->setProperty("gearEnable", true);

        heavyState->setProperty("sampleGain", 0.9f);
        heavyState->setProperty("samplePitch", 0.2f);
        heavyState->setProperty("sampleEnable", true);

        heavyState->setProperty("masterGain", 0.8f);

        heavyState->setProperty("macro1", 0.8f);
        heavyState->setProperty("macro2", 0.3f);
        heavyState->setProperty("macro3", 0.9f);
        heavyState->setProperty("macro4", 0.2f);

        factoryPresets.push_back({ "Heavy Mech", juce::var(heavyState.get()) });
    }

    // Light Scout preset
    {
        juce::DynamicObject::Ptr lightState = new juce::DynamicObject();
        lightState->setProperty("hydraulicIntensity", 0.3f);
        lightState->setProperty("hydraulicFilter", 0.9f);
        lightState->setProperty("hydraulicGain", 0.4f);
        lightState->setProperty("hydraulicEnable", true);

        lightState->setProperty("servoFreq", 0.8f);
        lightState->setProperty("servoModDepth", 0.2f);
        lightState->setProperty("servoGain", 0.6f);
        lightState->setProperty("servoEnable", true);

        lightState->setProperty("metalResonance", 0.4f);
        lightState->setProperty("metalDecay", 0.3f);
        lightState->setProperty("metalGain", 0.5f);
        lightState->setProperty("metalEnable", true);

        lightState->setProperty("gearRoughness", 0.2f);
        lightState->setProperty("gearSpeed", 0.8f);
        lightState->setProperty("gearGain", 0.4f);
        lightState->setProperty("gearEnable", true);

        lightState->setProperty("sampleGain", 0.5f);
        lightState->setProperty("samplePitch", 0.8f);
        lightState->setProperty("sampleEnable", true);

        lightState->setProperty("masterGain", 0.6f);

        lightState->setProperty("macro1", 0.3f);
        lightState->setProperty("macro2", 0.8f);
        lightState->setProperty("macro3", 0.2f);
        lightState->setProperty("macro4", 0.7f);

        factoryPresets.push_back({ "Light Scout", juce::var(lightState.get()) });
    }

    // Battle Damaged preset
    {
        juce::DynamicObject::Ptr damagedState = new juce::DynamicObject();
        damagedState->setProperty("hydraulicIntensity", 0.9f);
        damagedState->setProperty("hydraulicFilter", 0.1f);
        damagedState->setProperty("hydraulicGain", 0.8f);
        damagedState->setProperty("hydraulicEnable", true);

        damagedState->setProperty("servoFreq", 0.3f);
        damagedState->setProperty("servoModDepth", 0.9f);
        damagedState->setProperty("servoGain", 0.7f);
        damagedState->setProperty("servoEnable", true);

        damagedState->setProperty("metalResonance", 0.8f);
        damagedState->setProperty("metalDecay", 0.9f);
        damagedState->setProperty("metalGain", 0.9f);
        damagedState->setProperty("metalEnable", true);

        damagedState->setProperty("gearRoughness", 1.0f);
        damagedState->setProperty("gearSpeed", 0.6f);
        damagedState->setProperty("gearGain", 0.8f);
        damagedState->setProperty("gearEnable", true);

        damagedState->setProperty("sampleGain", 0.7f);
        damagedState->setProperty("samplePitch", 0.4f);
        damagedState->setProperty("sampleEnable", true);

        damagedState->setProperty("masterGain", 0.9f);

        damagedState->setProperty("macro1", 0.9f);
        damagedState->setProperty("macro2", 0.1f);
        damagedState->setProperty("macro3", 0.8f);
        damagedState->setProperty("macro4", 0.6f);

        factoryPresets.push_back({ "Battle Damaged", juce::var(damagedState.get()) });
    }
}
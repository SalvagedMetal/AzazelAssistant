#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <iostream>
#include <cassert>
#include <string>
#include <fstream>
#include <vector>

#include "nlohmann/json.hpp"

#include "configVars.h"
#include "model.h"
#include "mqtt.h"
#include "audio.h"

using json = nlohmann::json;

class ConfigReader {
private:
    std::string configData;
    json configJson;
    std::vector<ConfigVars::Model> models;
    ConfigVars::MQTTConfig mqtt;
    std::vector<ConfigVars::MQTTCommand> mqttCommands;
    std::vector<ConfigVars::AudioConfig> audio;
    ConfigVars::VoiceRecConfig voiceRec;
    ConfigVars::config config;
public:
    /*  
        \brief Reads and parses the configuration file at the given file path.
        \param filePath   Path to the configuration file
        \param isVerbose  Whether to print verbose output during reading and parsing
    */
    void readConfig(const std::string& filePath, const bool isVerbose);
    /*  
        Parses the configuration data read from the file.
        Populates the internal configuration structures.
    */
    void parseConfig();

    const ConfigVars::config getConfig() const;
    const std::vector<ConfigVars::Model> getModels() const;
    const ConfigVars::MQTTConfig getMQTTConfig() const;
    const std::vector<ConfigVars::Commands> getCommandCalls() const;
    const ConfigVars::VoiceConfig getVoiceConfig() const;
    const std::string getConfigData() const;
    const std::vector<ConfigVars::AudioConfig> getAudioConfig() const;
    const ConfigVars::VoiceRecConfig getVoiceRecConfig() const;
};

#endif

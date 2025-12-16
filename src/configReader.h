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

using json = nlohmann::json;

class ConfigReader {
private:
    std::string configData;
    json configJson;
    std::vector<ConfigVars::Model> models;
    ConfigVars::MQTTConfig mqtt;
    std::vector<ConfigVars::MQTTCommand> mqttCommands;
    ConfigVars::config config;

public:
    /*  
        Reads and parses the configuration file at the given file path.
        std::string& filePath   || Path to the configuration file
        bool isVerbose          || Whether to print verbose output during reading and parsing
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

};

#endif

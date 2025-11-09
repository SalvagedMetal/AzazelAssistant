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
    void readConfig(const std::string& filePath, const bool isVerbose);
    void parseConfig();

    const ConfigVars::config getConfig() const;
    const std::vector<ConfigVars::Model> getModels() const;
    const ConfigVars::MQTTConfig getMQTTConfig() const;
};
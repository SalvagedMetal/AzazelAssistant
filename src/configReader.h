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
    std::vector<ConfigVars::MQTTConfig> mqtt;
    std::vector<ConfigVars::MQTTCommand> mqttCommands;

public:
    void readConfig(const std::string&, const bool);
    void parseConfig();

    const std::vector<ConfigVars::Model> getModels() const;
    const std::vector<ConfigVars::MQTTConfig> getMQTT() const;
    const std::vector<ConfigVars::MQTTCommand> getMQTTCommand() const;
    const std::string getConfigData() const;
};
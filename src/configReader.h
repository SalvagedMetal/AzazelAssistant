#include <iostream>
#include <cassert>
#include <string>
#include <fstream>
#include <vector>

#include "nlohmann/json.hpp"

#include "configVars.h"

using json = nlohmann::json;

class ConfigReader {
private:
    std::string configData;
    json configJson;
    std::vector<ConfigVars::Model> models;

public:
    void readConfig(const std::string&);
    void parseConfig();

    const std::vector<ConfigVars::Model> getModels() const;
    const std::string getConfigData() const;
};
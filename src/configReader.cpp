#include "configReader.h"

void ConfigReader::readConfig(const std::string& filePath) {
    std::ifstream configFile(filePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open config file");
        return;
    }

    configFile >> configJson;
    configData = configJson.dump(4); // Pretty print the JSON
    std::cout << configJson.dump(4) << std::endl;
    if (configJson.is_null()) {
        throw std::runtime_error("Config file is empty or invalid");
    }
    configFile.close();
};

void ConfigReader::parseConfig() {
    if (configJson.is_null()) {
        throw std::runtime_error("Config JSON is null");
        return;
    }
    if (!configJson.contains("models") || !configJson["models"].is_array()) {
        throw std::runtime_error("Config JSON does not contain 'models' array");
        return;
   }

    for (const auto& modelJson : configJson["models"]) {
        if (!modelJson.is_object()) {
            throw std::runtime_error("Model entry is not an object");
        }
        ConfigVars::Model model;
        model.name = modelJson["name"].get<std::string>();
        model.purpose = modelJson["purpose"].get<std::string>();
        model.path = modelJson["path"].get<std::string>();
        model.ngl = modelJson["ngl"].get<int>();
        model.n_ctx = modelJson["n_ctx"].get<int>();
        model.temp = modelJson["temp"].get<float>();
        model.min_p = modelJson["min_p"].get<float>();
        model.top_p = modelJson["top_p"].get<float>();
        model.typical = modelJson["typical"].get<float>();
        model.top_k = modelJson["top_k"].get<int>();
        model.init_message = modelJson["init_message"].get<std::string>();
        model.keepHistory = modelJson["keepHistory"].get<bool>();
        if (modelJson["dist"].is_string() && modelJson["dist"] == "default")
            model.dist = LLAMA_DEFAULT_SEED; // Set default distribution if specified
        else
            model.dist = modelJson["dist"].get<float>();

        models.push_back(model);
    }
}

const std::vector<ConfigVars::Model> ConfigReader::getModels() const {
    return models;
}
const std::string ConfigReader::getConfigData() const {
    return configData;
}

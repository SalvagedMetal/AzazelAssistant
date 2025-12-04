#include "configReader.h"

void ConfigReader::readConfig(const std::string& filePath, bool verbose) {
    std::ifstream configFile(filePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open config file");
        return;
    }

    configFile >> configJson;
    configData = configJson.dump(4); // Pretty print the JSON
    if (verbose) {
        std::cout << "Config file read successfully: " << filePath << std::endl;
        std::cout << configJson.dump(4) << std::endl;
    }
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

    // Parse models
    if (!configJson.contains("models") || !configJson["models"].is_array()) {
        throw std::runtime_error("Config JSON does not contain 'models' array");
        return;
    }
    if (configJson.contains("ModelEnable")) {
        config.ModelEnable = configJson.value("ModelEnable", false);
    } else {
        config.ModelEnable = false; // Default to false if not specified
    }
    for (const auto& modelJson : configJson["models"]) {
        if (!modelJson.is_object()) {
            throw std::runtime_error("Model entry is not an object");
        }
        // Update to safe access with value() and default values later
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

        config.models.push_back(model);
    }

    // Parse MQTT configurations
    if (configJson.contains("mqtt") && configJson["mqtt"].is_object()) {
        const auto& mqttJson = configJson["mqtt"];

        config.mqtt.broker_ip = mqttJson.value("broker_ip", "localhost");
        config.mqtt.broker_port = mqttJson.value("broker_port", 1883);
        config.mqtt.username = mqttJson.value("username", "");
        config.mqtt.password = mqttJson.value("password", "");
        config.mqtt.client_id = mqttJson.value("client_id", "DefaultClient");
        config.mqtt.keepalive = mqttJson.value("keepalive", 60);
        config.mqtt.clean_session = mqttJson.value("clean_session", true);

        if (mqttJson.contains("commands") && mqttJson["commands"].is_array()) {
            for (const auto& cmdJson : mqttJson["commands"]) {
                ConfigVars::MQTTCommand cmd;

                cmd.name = cmdJson.value("name", "");
                cmd.type = cmdJson.value("type", "");
                cmd.topic = cmdJson.value("topic", "");
                cmd.qos = cmdJson.value("qos", 0);
                cmd.retain = cmdJson.value("retain", false);
                cmd.payload = cmdJson.value("payload", "");

                config.mqtt.commands.push_back(cmd);           
            }
        } else {
            throw std::runtime_error("MQTT config does not contain 'commands' array");
        }
    } else {
        throw std::runtime_error("Config JSON does not contain 'mqtt' object");
    }

    // Command Calling
    if (configJson.contains("commandCalls") && configJson["commandCalls"].is_array()) {
        for (const auto& cmdCallJson : configJson["commandCalls"]) {
            ConfigVars::Commands cmdCall;

            cmdCall.name = cmdCallJson.value("name", "");
            cmdCall.function = cmdCallJson.value("function", "");
            cmdCall.NArgs = cmdCallJson.value("NArgs", 0);
            cmdCall.confirmation = cmdCallJson.value("confirmation", false);
            cmdCall.priority = cmdCallJson.value("priority", 0);

            if (cmdCallJson.contains("phrases") && cmdCallJson["phrases"].is_array()) {
                for (const auto& phrase : cmdCallJson["phrases"]) {
                    cmdCall.phrases.push_back(phrase.get<std::string>());
                }
            } else {
                throw std::runtime_error("Command call does not contain 'phrases' array");
            }

            config.commandCalls.push_back(cmdCall);
        }
    } else {
        throw std::runtime_error("Config JSON does not contain 'commandCalls' array");
    }
}


const ConfigVars::config ConfigReader::getConfig() const {
    return config;
}

const std::vector<ConfigVars::Model> ConfigReader::getModels() const {
    return config.models;
}
const ConfigVars::MQTTConfig ConfigReader::getMQTTConfig() const {
    return config.mqtt;
}
const std::vector<ConfigVars::Commands> ConfigReader::getCommandCalls() const {
    return config.commandCalls;
}
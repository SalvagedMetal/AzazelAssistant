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
    if (!configJson.contains("models") || !configJson["models"].is_array())
        throw std::runtime_error("Config JSON does not contain 'models' array");
    if (configJson.contains("modelEnabled")) {
        config.ModelEnable = configJson.value("modelEnabled", false);
    } else {
        config.ModelEnable = false; // Default to false if not specified
    }
    for (const auto& modelJson : configJson["models"]) {
        if (!modelJson.is_object())
            throw std::runtime_error("Model entry is not an object");

        ConfigVars::Model model;
        model.name = modelJson.value("name", "");
        model.purpose = modelJson.value("purpose", "");
        model.path = modelJson.value("path", "");
        model.ngl = modelJson.value("ngl", 0);
        model.n_ctx = modelJson.value("n_ctx", 512);
        model.temp = modelJson.value("temp", 0.7f);
        model.min_p = modelJson.value("min_p", 0.0f);
        model.top_p = modelJson.value("top_p", 1.0f);
        model.typical = modelJson.value("typical", 1.0f);
        model.top_k = modelJson.value("top_k", 40);
        model.init_message = modelJson.value("init_message", "You are a helpful assistant.");
        model.keepHistory = modelJson.value("keepHistory", false);
        if (modelJson["dist"].is_string() && modelJson["dist"] == "default")
            model.dist = LLAMA_DEFAULT_SEED; // Set default distribution if specified
        else
            model.dist = modelJson.value("dist", LLAMA_DEFAULT_SEED);

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
    // TTS Voice
    if (configJson.contains("voice") && configJson["voice"].is_object()) {
        const auto& voiceJson = configJson["voice"];
        
        config.voice.enabled = voiceJson.value("enabled", false);
        config.voice.name = voiceJson.value("name", "default");
        config.voice.model_path = voiceJson.value("model_path", "");
        config.voice.config_path = voiceJson.value("config_path", "");
        config.voice.espeak_data_path = voiceJson.value("espeak_path", "");
        config.voice.sample_rate = voiceJson.value("sample_rate", 22050);
        config.voice.output_file = voiceJson.value("output_file", "output.raw");
        config.voice.length_scale = voiceJson.value("length_scale", 1.0f);
        config.voice.noise_scale = voiceJson.value("noise_scale", 0.667f);
        config.voice.noise_w_scale = voiceJson.value("noise_w_scale", 0.8f);
        
    } else {
        throw std::runtime_error("Config JSON does not contain 'voice' object");
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

const ConfigVars::VoiceConfig ConfigReader::getVoiceConfig() const {
    return config.voice;
}
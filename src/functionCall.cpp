#include "functionCall.h"
#include "mqttQueue.h"

using json = nlohmann::json;
using namespace nlohmann::literals;

std::string FunctionCall::call(const std::string command, Model& model, const bool isVerbose, MQTTClient& mqtt, ConfigVars::config& config) {
    json parsedCommand;
    try {
        parsedCommand = json::parse(command);
    } catch (const json::parse_error &e) {
        throw std::invalid_argument("Invalid JSON format: " + std::string(e.what()));
    }

    // Process the command
    if (isVerbose) {
        std::cout << "Executing command: " << parsedCommand.dump() << std::endl;
    }

    std::string function = parsedCommand["command"];
    if (function == "chat") {
        if (parsedCommand.contains("arguments") && parsedCommand["arguments"].is_array()) {
            const auto& args = parsedCommand["arguments"];
            if (args.size() == 1 && args[0].is_string()) {
                std::string userPrompt = args[0];
                if (isVerbose) {
                    std::cout << "User prompt: " << userPrompt << std::endl;
                }
                return model.respond(parsedCommand["arguments"][0].get<std::string>());
            } else {
                throw std::invalid_argument("Invalid arguments for chat command");
            }
        }
    } else if (function == "GetCurrentTime") {
        return DateTime::getCurrentDateTime(DTFormat::HHMMSS24);
    } else if (function == "GetCurrentDate") {
        return DateTime::getCurrentDateTime(DTFormat::DDMMYYYY);
    } else if (function == "testPublish") {
        if (isVerbose) {
            std::cout << "Starting MQTT Publish task..." << std::endl;
        }
        for (const auto& cmd : config.mqtt.commands) {
            if (cmd.name == "testPublish") {
                try {
                    mqtt.Publish(nullptr, cmd.payload,
                        cmd.payload.length(), cmd.topic, cmd.qos, cmd.retain);
                    return "Publish successful";
                } catch (const std::exception &e) {
                    return std::string("Publish error: ") + e.what();
                }
            }
        }
    } else if (function == "testSubscribe") {
        if (isVerbose) {
            std::cout << "Starting MQTT Subscribe task..." << std::endl;
        }
        for (const auto& cmd : config.mqtt.commands) {
            if (cmd.name == "testSubscribe") {
                try {
                    std::string result = mqtt.Subscribe(cmd.topic, cmd.qos);
                    return result;
                } catch (const std::exception &e) {
                    return std::string("Subscribe error: ") + e.what();
                }
            }
        }
    } else {
        throw std::invalid_argument("Unknown command: " + function);
    }
    return "";
}


bool FunctionCall::isValidCommand(const std::string command, const bool isVerbose) {
    json parsedCommand = json::parse(command);
    if (!parsedCommand.is_object()) {
        throw std::invalid_argument("Invalid command format: not a JSON object");
        return false;
    }

    for (const auto& cmd : FunctionCall::commands) {
        if (parsedCommand.contains("command") && parsedCommand["command"] == cmd.function) {
            if (parsedCommand.contains("arguments") && parsedCommand["arguments"].is_array()) {
                const auto& args = parsedCommand["arguments"];
                if (args.size() == cmd.NArgs) {
                    if (isVerbose) {
                        std::cout << "Valid command: " << cmd.function << std::endl;
                    }  
                    return true;
                } else {
                    throw std::invalid_argument("Invalid number of arguments for command: " + cmd.function);
                    return false;
                }
            } else {
                throw std::invalid_argument("Missing or invalid 'arguments' field in command: " + cmd.function);
                return false;
            }
        } else {
            if (isVerbose) {
                std::cout << "Command not found: " << parsedCommand["command"] << std::endl;
            }
        }
    }
    return false;
}
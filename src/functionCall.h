#ifndef FUNCTIONCALL_H
#define FUNCTIONCALL_H

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <typeinfo>

#include "nlohmann/json.hpp"

#include "model.h"
#include "dateTime.h"
#include "mqtt.h"
#include "configReader.h"

namespace FCFormat {
    struct ModelCommand {
        std::string function;
        int NArgs;
        std::vector<std::string> args;
    };

    struct MQTTTask {
        std::string type; // "publish" or "subscribe"
        std::string topic;
        std::string message;
        int qos;
        bool retain;
    };
}

namespace FunctionCall {

    // List of valid commands
    const std::vector<FCFormat::ModelCommand> commands = {
        {"GetCurrentTime", 0, {"format"}},
        {"GetCurrentDate", 0, {"format"}},
        {"chat", 1, {"userPrompt"}},
        {"testPublish", 0},
        {"testSubscribe", 0}
    };

    // list of commands
    std::string call(const std::string command, Model& model, const bool isVerbose, MQTTClient& mqtt, ConfigVars::config& config);
    bool isValidCommand(const std::string command, const bool isVerbose);
}

#endif
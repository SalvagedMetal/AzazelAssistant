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

namespace FCFormat {
    struct ModelCommand {
        std::string function;
        int NArgs;
        std::vector<std::string> args;
    };
}

namespace FunctionCall {
    // List of valid commands
    const std::vector<FCFormat::ModelCommand> commands = {
        {"GetCurrentTime", 0, {"format"}},
        {"GetCurrentDate", 0, {"format"}},
        {"chat", 1, {"userPrompt"}}
    };

    // list of commands
    std::string call(const std::string, Model&, const bool);
    bool isValidCommand(const std::string, const bool);
}

#endif
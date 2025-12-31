#ifndef FUNCTIONCALL_H
#define FUNCTIONCALL_H

#include <vector>
#include <string>
#include <functional>
#include <any>
#include <memory>

// Dummy declarations
class Model;
class MQTTClient;
class Voice;
namespace ConfigVars {
    struct Model;
    struct MQTTCommand;
    struct MQTTConfig;
    struct Commands;
    struct config;
}

namespace FunctionCall {

    //difference allowed to be conisidered a typo
    const float ratio = 0.3;

    struct Command {
        std::string command;
        int NArgs;
        std::vector<std::string> argTypes;
        std::any cntx;
        std::function<std::string(const std::vector<std::string>&)> function;
    };

    struct ParsedPhrase {
        std::string command;
        std::vector<std::string> arguments;
    };

    extern std::vector <FunctionCall::Command> commandList;

    // List of phrases
    const std::vector<std::string> ignorePatterns = {
        "please",
        "could",
        "would",
        "kindly",
        "you",
        "can"
    };
    const std::vector<std::string> ignoreSymbols = {
        ",",
        ".",
        "!",
        "?",
        "%"
    };

    // list of commands
    /* FunctionCall::Call to call a function by its ParsedPhrase
        std::unique_ptr<FunctionCall::ParsedPhrase>& ParsedCommand      || Parsed command to execute
        ConfigVars::config& config                                      || Configuration variables
        const bool isVerbose                                            || Whether to print verbose output
        returns                                                         || Result of the command execution as a string
    */
    std::string call(const std::unique_ptr<FunctionCall::ParsedPhrase>& ParsedCommand, ConfigVars::config& config, const bool isVerbose);
    /* FunctionCall::ParsePhrase to parse a phrase into a ParsedPhrase
        const std::string phrase                                        || Input phrase to parse
        std::unique_ptr<FunctionCall::ParsedPhrase>& outParsed          || Output parsed phrase
        const std::vector<ConfigVars::Commands>& commands               || List of available commands
        const bool isVerbose                                            || Whether to print verbose output
        returns                                                         || true if parsing was successful, false otherwise
    */
    bool parsePhrase(const std::string phrase, std::unique_ptr<FunctionCall::ParsedPhrase>& outParsed, const std::vector<ConfigVars::Commands>& commands, const bool isVerbose);
    /* FunctionCall::CheckTypo to check if two strings are similar enough to be considered a typo
        const std::string& string1                                      || First string to compare
        const std::string& string2                                      || Second string to compare
        const float ratio                                               || Maximum allowed difference ratio
        const bool isVerbose                                            || Whether to print verbose output
        returns                                                         || true if the strings are similar enough, false otherwise
    */
    bool checkTypo(const std::string& string1, const std::string& string2, const float ratio, const bool isVerbose);
    /* FunctionCall::initCommands to initialize the command list
        const ConfigVars::config& config                                || Configuration variables
        MQTTClient* client                                              || Pointer to MQTT client instance
        Model* model                                                    || Pointer to Model instance
        const bool isVerbose                                            || Whether to print verbose output
    */
    void initCommands(const ConfigVars::config& config, MQTTClient* client, Model* model, Voice* voice, const bool isVerbose);
}

#endif

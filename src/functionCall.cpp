#include "functionCall.h"
#include "nlohmann/json.hpp"
#include "model.h"
#include "dateTime.h"
#include "mqtt.h"
#include "configReader.h"

using json = nlohmann::json;
using namespace nlohmann::literals;

// Using levenshtien distance to check if there is a typo or not
bool FunctionCall::checkTypo(const std::string& string1, const std::string& string2, const float ratio, const bool isVerbose) {
    if (ratio > 1) {
        throw std::invalid_argument("Ratio is higher than 1: " + std::to_string(ratio));
    }

    std::vector<int> prevRow(string2.length() + 1, 0);
    std::vector<int> currRow(string2.length() + 1, 0);

    for (int j = 0; j <= string2.length(); j++) prevRow[j] = j;

    for (int i = 1; i <= string1.length(); i++) {
        currRow[0] = i;

        for (int j = 1; j <= string2.length(); j++) {
            if (string1[i - 1] == string2[j - 1]) {
                currRow[j] = prevRow[j - 1];
            } else {
                currRow[j] = 1 + std::min(currRow[j - 1], std::min(prevRow[j], prevRow[j - 1]));
            }
        }
        prevRow = currRow;
    }
    float averageLen = (float)(string1.length() + string2.length()) / 2;
    
    if (isVerbose) std::cout << "diffrences: " << currRow[string2.length()] << ", average length: " << std::to_string(averageLen) <<  std::endl;
    if ((currRow[string2.length()] / averageLen) <= FunctionCall::ratio) return true; // Typo detected
    return false; // Difference too large to be considered a typo
}


std::string FunctionCall::call(const std::unique_ptr<FunctionCall::ParsedPhrase>& ParsedCommand, Model& model, MQTTClient& mqtt, ConfigVars::config& config, const bool isVerbose) {
    for (const auto& cmd : commandList) {
        if (ParsedCommand->command == cmd.command) {
            if (ParsedCommand->arguments.size() != static_cast<size_t>(cmd.NArgs)) {
                throw std::invalid_argument("Invalid number of arguments for command: " + cmd.command);
            }
            // Confirm request
            for (const auto& confCmd : config.commandCalls) {
                if ((confCmd.name == cmd.command) && confCmd.confirmation) {
                    std::string userInput;
                    std::cout << "Are you sure you want to execute the command '" << cmd.command << "' with " << ParsedCommand->arguments.size() << " arguments? (yes/no): ";
                    std::getline(std::cin, userInput);
                    std::transform(userInput.begin(), userInput.end(), userInput.begin(), ::tolower);
                    if (userInput != "yes" && userInput != "y") {
                        return "Command '" + cmd.command + "' cancelled by user.";
                    }
                }
            }
            if (isVerbose) std::cout << "Executing command: " << cmd.command << std::endl;
            std::string response = cmd.function(ParsedCommand->arguments);
            return response;
        }
    }
    return "Could not find command";
}


bool FunctionCall::parsePhrase(const std::string phrase, std::unique_ptr<FunctionCall::ParsedPhrase>& outParsed, const std::vector<ConfigVars::Commands>& commands, const bool isVerbose) {
    auto toLower = [](std::string s){
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };

    auto splitWords = [](const std::string &s){
        std::vector<std::string> words;
        std::istringstream iss(s);
        std::string w;
        while (iss >> w) words.push_back(w);
        return words;
    };

    for (const auto& cmd : commands) {
        if (isVerbose) std::cout << "Checking command: " << cmd.name << std::endl;
        for (const auto& pattern : cmd.phrases) {
            if (isVerbose) std::cout << "Comparing input: " << phrase << " with pattern: " << pattern << std::endl;

            std::vector<std::string> patternWords = splitWords(pattern);

            // Determine whether this pattern contains a "rest" argument (ends with "->")
            bool patternHasRest = false;
            for (const auto &pw : patternWords) {
                if (pw.find("<arg") != std::string::npos && pw.size() >= 2 && pw.substr(pw.size() - 2) == "->") {
                    patternHasRest = true;
                    break;
                }
            }

            // Tokenize and clean phrase according to pattern rules
            std::string lowerPhrase = toLower(phrase);
            std::vector<std::string> phraseWords;
            for (auto w : splitWords(lowerPhrase)) {
                for (const auto& sym : ignoreSymbols) {
                    size_t p;
                    while ((p = w.find(sym)) != std::string::npos) w.erase(p, 1);
                }
                if (!patternHasRest && std::find(ignorePatterns.begin(), ignorePatterns.end(), w) != ignorePatterns.end()) {
                    continue;
                }
                if (!w.empty()) phraseWords.push_back(w);
            }

            if (!patternHasRest && (phraseWords.size() != patternWords.size())) {
                if (isVerbose) std::cout << "Phrase shorter than pattern, skipping\n";
                continue;
            }

            // Compare words
            bool matched = true;
            outParsed = nullptr;
            for (size_t i = 0; i < patternWords.size(); ++i) {
                const auto &pword = patternWords[i];

                // rest-argument placeholder
                if (pword.find("<arg") != std::string::npos && pword.size() >= 2 && pword.substr(pword.size() - 2) == "->") {
                    if (isVerbose) std::cout << "Found rest argument placeholder: " << pword << std::endl;
                    int argIndex = std::stoi(pword.substr(4, pword.size() - 6)); // remove <arg and ->
                    if (argIndex < cmd.NArgs) {
                        if (!outParsed) outParsed = std::make_unique<FunctionCall::ParsedPhrase>();
                        outParsed->command = cmd.function;

                        std::string rest;
                        for (size_t j = i; j < phraseWords.size(); ++j) {
                            if (j > i) rest += " ";
                            rest += phraseWords[j];
                        }
                        outParsed->arguments.push_back(rest);
                        if (isVerbose) std::cout << "Parsed rest of phrase for argument " << argIndex << ": " << rest << std::endl;
                    }
                    matched = true;
                    break;
                }

                // normal argument placeholder
                if (pword.find("<arg") != std::string::npos) {
                    if (isVerbose) std::cout << "Found argument placeholder: " << pword << std::endl;
                    int argIndex = std::stoi(pword.substr(4, pword.length() - 5));
                    if (argIndex < cmd.NArgs) {
                        // missing word for argument
                        if (i >= phraseWords.size()) { 
                            matched = false;
                            break;
                        }
                        if (!outParsed) outParsed = std::make_unique<FunctionCall::ParsedPhrase>();
                        outParsed->command = cmd.function;
                        outParsed->arguments.push_back(phraseWords[i]);
                        if (isVerbose) std::cout << "Parsed argument " << argIndex << ": " << phraseWords[i] << std::endl;
                    }
                    continue;
                }

                // literal word: compare (case-insensitive)
                if (i >= phraseWords.size()) {
                    matched = false;
                    break;
                }
                std::string lp = toLower(pword);
                if (lp != phraseWords[i]) {
                    if (!checkTypo(lp, phraseWords[i], ratio, isVerbose)) {
                        matched = false;
                        break;
                    } else if (isVerbose) {
                        std::cout << "Accepted Typo: " << phraseWords[i] << std::endl;
                    }
                }
            }

           if (matched) {
                if (isVerbose) std::cout << "Pattern matched: " << pattern << std::endl;
                
                // If outParsed has no arguments
                if (!outParsed) {
                    outParsed = std::make_unique<FunctionCall::ParsedPhrase>();
                    outParsed->command = cmd.function;
                }
                if (!outParsed) {
                    return false;
                }
                return true;
            }
        }
    }

    std::cout << "No matching command pattern found for phrase: " << phrase << std::endl;
    outParsed = nullptr;
    return false;
}

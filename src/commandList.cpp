#include "functionCall.h"
#include "model.h"
#include "dateTime.h"
#include "mqtt.h"
#include "configReader.h"

std::vector <FunctionCall::Command> FunctionCall::commandList;

void FunctionCall::initCommands(const ConfigVars::config& config, MQTTClient* mqttClient, Model* model, const bool isVerbose) {
    commandList.clear();
    ConfigVars::MQTTConfig mqttVars = config.mqtt;

    if (isVerbose) std::cout << "Pushing getCurrentDateTime" << std::endl;
    commandList.push_back({
        "getCurrentDateTime", 1, {"String"}, nullptr,
        [isVerbose](const std::vector<std::string>& args) -> std::string {
            if (isVerbose) std::cout << "Running getCurrentDateTime" << std::endl;
            if (args.empty()) {
                throw std::invalid_argument("Missing argument");
            }
            if (args[0] == "time" || checkTypo("time", args[0], FunctionCall::ratio, false)) {
                return "It is " + DateTime::getCurrentDateTime(DTFormat::HHMMSS24);
            } else if (args[0] == "date" || checkTypo("date", args[0], FunctionCall::ratio, false)) {
                return "Today is " + DateTime::getCurrentDateTime(DTFormat::DDMMYYYY);
            } else if (args[0] == "day" || checkTypo("day", args[0], FunctionCall::ratio, false)) {
                return "Today is " + DateTime::getCurrentDateTime(DTFormat::Day);
            } else {
                std::cerr << "Invalid argument for GetCurrentDateTime: " << args[0];
                return "Sorry, I didn't get that";
            }
    }});

    if (isVerbose) std::cout << "Pushing getDateTime" << std::endl;
    commandList.push_back({
        "getDateTime", 3, {"String", "Integer", "String"}, nullptr,
        [isVerbose](const std::vector<std::string>& args) -> std::string {
            if (isVerbose) std::cout << "Running getCurrentDateTime" << std::endl;
            if (args.size() < 2) {
                throw std::invalid_argument("Missing arguments for GetDateTime");
            }

            std::string what = args[0];   // "time", "date", "day"
            std::string a1   = args[1];   // "3" or "tuesday"
            std::string a2   = args.size() > 2 ? args[2] : ""; // "days", "hours", etc.

            // Output format
            const char* outputFormat = nullptr;

            if (what == "time" || checkTypo("time", what, FunctionCall::ratio, false)) {
                outputFormat = DTFormat::HHMMSS24;
            } else if (what == "date" || checkTypo("date", what, FunctionCall::ratio, false)) {
                outputFormat = DTFormat::DDMMYYYY;
            } else if (what == "day"  || checkTypo("day",  what, FunctionCall::ratio, false)) {
                outputFormat = DTFormat::Day;
            } else {
                return "Sorry, I don’t understand what you want (" + what + ")";
            }

            // Current timestamp
            time_t now = DateTime::getCurrentTimestamp();
            struct tm current = *localtime(&now);

            // arg1 is a weekday
            static const std::vector<std::string> weekdays = {
                "sunday","monday","tuesday","wednesday",
                "thursday","friday","saturday"
            };

            {
                std::string a1lower = a1;
                std::transform(a1lower.begin(), a1lower.end(), a1lower.begin(), ::tolower);

                for (int i = 0; i < 7; i++) {
                    if (a1lower == weekdays[i] || checkTypo(weekdays[i], a1lower, FunctionCall::ratio, false)) {

                        int today = current.tm_wday;
                        int diff = (i - today + 7) % 7;
                        if (diff == 0) diff = 7; // next week

                        now += diff * 24 * 3600;
                        return DateTime::getDateTime(now, outputFormat);
                    }
                }
            }
            // arg1 is numeric
            bool isNumber = std::all_of(a1.begin(), a1.end(), ::isdigit);

            if (isNumber) {
                long amount = std::stol(a1);
                std::string unit = a2;
                std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);

                if (unit == "day" || unit == "days") {
                    now += amount * 24 * 3600;
                } else if (unit == "hour" || unit == "hours") {
                    now += amount * 3600;
                } else if (unit == "week" || unit == "weeks") {
                    now += amount * 7 * 24 * 3600;
                } else if (unit == "month" || unit == "months") {
                    current.tm_mon += amount;
                    now = mktime(&current);
                } else {
                    return "Sorry, I don’t understand the unit (" + unit + ")";
                }

                return DateTime::getDateTime(now, outputFormat);
            }
            // date parsing
            try {
                std::string format = DateTime::findFormat(a1);
                time_t ts = DateTime::getTimestamp(a1.c_str(), format.c_str());
                return DateTime::getDateTime(ts, outputFormat);
            } catch (const std::exception &e) {
                return "Sorry, I couldn’t understand the date \"" + a1 + "\"";
            }
        }
    });

    if (config.mqtt.enabled) {

        for (const auto& configCommands : config.commandCalls) {
            for (const auto& mqtt : mqttVars.commands) {
                if (mqtt.name == configCommands.function) {
                    if (isVerbose) std::cout << "Pushing MQTT command: " << mqtt.name << std::endl;
                    commandList.push_back({
                        configCommands.name, 0, {}, mqttClient,
                        [mqttClient, mqtt, isVerbose](const std::vector<std::string>& args) -> std::string {
                            if (isVerbose) std::cout << "Running MQTT " << mqtt.name << std::endl;
                            if (mqtt.type == "publish") {
                                try {
                                    mqttClient->Publish(nullptr, mqtt.payload, mqtt.payload.length(), mqtt.topic, mqtt.qos, mqtt.retain);
                                    return "Publish successful";
                                } catch (const std::exception &e) {
                                    return std::string("Publish error: ") + e.what();
                                }
                            } else if (mqtt.type == "subscribe") {
                                try {
                                    return mqttClient->Subscribe(mqtt.topic, mqtt.qos);
                                } catch (const std::exception& e) {
                                    return std::string("Subscribe error: ") + e.what();
                                }
                            } else {
                                throw std::invalid_argument("Could not find valid type for " + mqtt.name + " type: " + mqtt.type);
                                return "INVALID ARGUMENT";
                            }
                        }
                    });
                }
            }
        }
    }
    
    if (config.ModelEnable) {

        if (isVerbose) std::cout << "Pushing chat command" << std::endl;
        commandList.push_back({
            "chat", 1, {"String"}, model,
            [model, isVerbose](const std::vector<std::string>& args) -> std::string {
                std::string response = "";
                if (isVerbose) std::cout << "Running chat with user prompt: " << args[0] << std::endl;
                if (args.empty()) {
                    throw std::invalid_argument("No arguments detected");
                }
                try {
                    if (isVerbose) std::cout << "Generating response from model..." << std::endl;
                        response = model->respond(args[0]);
                } catch (const std::exception &e) {
                    std::cerr << "Error generating response from model: " << e.what() << std::endl;
                    return "Error generating response from model: " + std::string(e.what());
                }
                return response;
            }
        });
    }
}
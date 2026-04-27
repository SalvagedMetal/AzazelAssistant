#include "src/model.h"
#include "src/functionCall.h"
#include "src/mqtt.h"
#include "src/configReader.h"
#include "src/voice.h"
#include "src/audio.h"

#include <thread>
#include <mutex>
#include <condition_variable>


//shared thread objects
std::mutex tts_mtx;
std::condition_variable tts_cv;

int main(int argc, char *argv[]) {

    std::string response;
    std::string commandString;
    std::string userInput, input;
    std::string commandInitMessage;
    bool isVerbose = false;
    bool retry = false;
    bool ttsEnabled = true;

    ConfigVars::config config;
    ConfigVars::MQTTConfig mqttConfig;
    MQTTClient client;
    Model commandModel;
    Model chatModel;
    std::unique_ptr<FunctionCall::ParsedPhrase> parsedPhrasePtr = nullptr;
    ConfigReader configReader;
    Voice voice;
    InputAudio recordVoice;
    OutputAudio ttsPlayback;
    std::queue<std::vector<float>> theVoices;



    // Processing command line arguments
    for (int i = 0; i < argc; ++i) {
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
            std::cout << "Usage: AzazelAssistant [options]\n"
                      << "Options:\n"
                      << " --help, -h       Show this help message\n"
                      << " --versbose, -v   Enable verbose output\n"
                      << " --silent, -s     Run in silent mode (no TTS output)\n";
            return 0;
        } else if (std::string(argv[i]) == "--verbose" || std::string(argv[i]) == "-v") {
            std::cout << "Verbose mode enabled\n";
            isVerbose = true;
        } else if (std::string(argv[i]) == "--silent" || std::string(argv[i]) == "-s") {
            std::cout << "Silent mode enabled (no TTS output)\n";
            ttsEnabled = false;
        }
    }
    // Load configuration
    try {
        configReader.readConfig("config.json", isVerbose); 
        configReader.parseConfig();
    } catch (const std::exception &e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return 1;
    }
    config = configReader.getConfig();

    // Initialize MQTT client
    mqttConfig = configReader.getMQTTConfig();

    if (mqttConfig.enabled) {
        client.setVerbose(isVerbose);
        if (mqttConfig.enabled) {
            try {
                std::cout << "Initialising MQTT client... ";
                client.Init(mqttConfig.username, mqttConfig.password, mqttConfig.client_id, mqttConfig.clean_session);
                if (client.isInitialized()) {
                    client.Start(mqttConfig.broker_ip, mqttConfig.broker_port, mqttConfig.keepalive);
                } else {
                    throw std::runtime_error("MQTT client initialization failed.");
                }
            } catch (const std::exception &e) {
                std::cerr << "Error initializing MQTT client: " << e.what() << std::endl;
                return 1;
            }
        }
        std::cout << "Done." << std::endl;
    }

    // Initialise audio
    if (config.audio.enabled) {
        std::cout << "Initialising audio... ";
        recordVoice.setChannels(config.audio.channels);
        recordVoice.setDuration(5.0f);
        recordVoice.setGain(config.audio.gain);
        recordVoice.setSampleRate(config.audio.sampleRate);
        recordVoice.setIsVerbose(isVerbose);
        
        ttsPlayback.setChannels(config.audio.channels);
        ttsPlayback.setSampleRate(config.voice.sample_rate);
        ttsPlayback.setIsVerbose(isVerbose);
        try {
            recordVoice.init();
            ttsPlayback.init();
        } catch (const std::exception &e) {
            std::cerr << "Error initialising audio: " << e.what() << std::endl;
            return 1;
        }
        std::cout << "Done." << std::endl;
    }

    // Initialize the models
    // Only initialize models if enabled in config
    if (config.ModelEnable) {
        // Phrases in a single string to add to the model init message
        std::cout << "Initialising models... ";
        std::string phrasesString = "";
        for (const auto& command : configReader.getCommandCalls()) {
            for (const auto& phrase : command.phrases) {
                phrasesString += phrase + "\n";
            }
        }
        std::vector<ConfigVars::Model> commandModelConfig = configReader.getModels();
        if (commandModelConfig.empty()) {
            std::cerr << "No models defined in configuration." << std::endl;
            return 1;
        }
        for (const auto& modelConfig : commandModelConfig) {
            if (modelConfig.purpose == "Command") {
                // Prepend phrases to init message
                commandInitMessage = modelConfig.init_message + "\n" + phrasesString;
            }
        }

        for (const auto& modelConfig : configReader.getModels()) {
            if (modelConfig.purpose == "Command") {
                commandModel.setModelName(modelConfig.name);
                commandModel.setModelPurpose(modelConfig.purpose);
                commandModel.setModelPath(modelConfig.path);
                commandModel.setNGL(modelConfig.ngl);
                commandModel.setNCTX(modelConfig.n_ctx);
                commandModel.setInitMessage(commandInitMessage);
                commandModel.setTemp(modelConfig.temp);
                commandModel.setMinP(modelConfig.min_p);
                commandModel.setTopP(modelConfig.top_p);
                commandModel.setTypical(modelConfig.typical);
                commandModel.setDist(modelConfig.dist);
                commandModel.setTopK(modelConfig.top_k);
                commandModel.setKeepHistory(modelConfig.keepHistory);
                commandModel.setVerbose(isVerbose);
                if (isVerbose) std::cout << "Command Model initialized: " << commandModel.getModelName() << std::endl;
            } else if (modelConfig.purpose == "Chat") {
                chatModel.setModelName(modelConfig.name);
                chatModel.setModelPurpose(modelConfig.purpose);
                chatModel.setModelPath(modelConfig.path);
                chatModel.setNGL(modelConfig.ngl);
                chatModel.setNCTX(modelConfig.n_ctx);
                chatModel.setInitMessage(modelConfig.init_message);
                chatModel.setTemp(modelConfig.temp);
                chatModel.setMinP(modelConfig.min_p);
                chatModel.setTopP(modelConfig.top_p);
                chatModel.setTypical(modelConfig.typical);
                chatModel.setDist(modelConfig.dist);
                chatModel.setTopK(modelConfig.top_k);
                chatModel.setKeepHistory(modelConfig.keepHistory);
                chatModel.setVerbose(isVerbose);
                if (isVerbose) std::cout << "Chat Model initialized: " << chatModel.getModelName() << std::endl;
            }
        }
        try {
            commandModel.init();
            chatModel.init();
        } catch (const std::exception &e) {
            std::cerr << "Error initialising command model: " << e.what() << std::endl;
            return 1;
        }
        std::cout << "Done." << std::endl;
    }

    // Initialize TTS voice synthesizer
    if (ttsEnabled && config.voice.enabled) {
        std::cout << "Initialising voice synthesiser... ";
        // setting voice config
        voice.setModelPath(config.voice.model_path);
        voice.setConfigPath(config.voice.config_path);
        voice.setEspeakDataPath(config.voice.espeak_data_path);
        voice.setSampleRate(config.voice.sample_rate);
        voice.setChannels(1);
        voice.setLengthScale(config.voice.length_scale);
        voice.setNoiseScale(config.voice.noise_scale);
        voice.setNoiseWScale(config.voice.noise_w_scale);
        voice.setGain(config.voice.gain);
        voice.setVerbose(isVerbose);
        try {
            voice.init();
        } catch (const std::exception &e) {
            std::cerr << "Error initialising voice synthesiser: " << e.what() << std::endl;
            return 1;
        }
        std::cout << "Done." << std::endl;
    }

    // Initialize function calls
    try {
        std::cout << "Initialising function calls... ";
        FunctionCall::initCommands(config, &client, &chatModel, &voice, &recordVoice, &ttsPlayback, isVerbose);
    } catch (const std::exception &e) {
        std::cerr << "Error initialising function calls: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Done." << std::endl;


    // Set up thread loops
    std::thread ttsThread([&ttsPlayback, &theVoices] {
        while(true) {
            std::vector<float> playBack;
            {
                std::unique_lock<std::mutex> lock(tts_mtx);
                tts_cv.wait(lock, [&theVoices] { return !theVoices.empty(); });

                playBack = std::move(theVoices.front());
                theVoices.pop();
            }
            ttsPlayback.playAudioBuffer(playBack);
        }
    });

    std::cout << "Azazel Assistant v0.4 is running...\n";


    // Main loop
    while (true) {
        if (!retry || !config.ModelEnable) {
            std::cout << "> ";
            getline(std::cin, userInput);
            input = userInput;
            if (userInput == "quit" || userInput == "q") break;
        } else {
            if (isVerbose) std::cout << "Retrying with AI parsed command..." << std::endl;
            try {
                    input = commandModel.respond(userInput);
            } catch (const std::exception &e) {
                std::cerr << "Error generating command from AI: " << e.what() << std::endl;
                return 1;
            }
            if (isVerbose) std::cout << "AI parsed command: " << input << std::endl;
        }
        if (FunctionCall::parsePhrase(input, parsedPhrasePtr, configReader.getCommandCalls(), isVerbose)) {
            if (isVerbose) {
                std::cout << "Command: " << parsedPhrasePtr->command << std::endl;
                for (const auto& arg : parsedPhrasePtr->arguments) {
                    std::cout << "Argument: " << arg << std::endl;
                }
            }
            response = FunctionCall::call(parsedPhrasePtr, config, isVerbose);
            std::cout << response << std::endl;
            if (ttsEnabled && config.voice.enabled) {
                if (!response.empty()) {
                    try {
                        voice.synthesise(response);
                        std::lock_guard<std::mutex> lock(tts_mtx);
                        theVoices.push(voice.getAudioData());
                        if (isVerbose) {
                            voice.saveToFile("Output.wav");
                            std::cout << "saving sythesis to file" << std::endl;
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "Error during TTS synthesis: " << e.what() << std::endl;
                    }
                    tts_cv.notify_one();
                }
            }
            retry = false;
        } else if (config.ModelEnable && !retry) {
            retry = true;
        } else {
            std::cout << "Could not parse command." << std::endl;
            if (ttsEnabled && config.voice.enabled) {
                try {
                    voice.synthesise("Could not parse command");
                        std::lock_guard<std::mutex> lock(tts_mtx);
                        theVoices.push(voice.getAudioData());
                        if (isVerbose) {
                            voice.saveToFile("Output.wav");
                            std::cout << "saving sythesis to file" << std::endl;
                        }
                } catch (const std::exception &e) {
                    std::cerr << "Error during TTS synthesis: " << e.what() << std::endl;
                }
                tts_cv.notify_one();
            }
            retry = false;
        }
        input = "";
        parsedPhrasePtr = nullptr;
        response = "";
    }
    ttsThread.join();
    return 0;
}

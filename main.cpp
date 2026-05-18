#include "src/model.h"
#include "src/functionCall.h"
#include "src/mqtt.h"
#include "src/configReader.h"
#include "src/voice.h"
#include "src/audio.h"
#include "src/voiceRec.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>


//shared thread objects
std::mutex tts_mtx, rec_mtx, trnscpt_mtx, command_mtx, streamAudio_mtx;
std::condition_variable tts_cv, rec_cv, trnscpt_cv, command_cv, streamAudio_cv;

int main(int argc, char *argv[]) {

    std::string response;
    std::string commandString;
    std::string userInput, input;
    std::string commandInitMessage;
    bool isVerbose = false;
    std::atomic<bool> aRetry = false;
    bool StreamAudio = true;
    bool ttsEnabled = true;
    std::atomic<bool> wakeWord = false;
    std::queue<std::vector<float>> theVoices;
    std::queue<std::vector<float>> voiceRecording;
    std::queue<std::string> commands;
    std::vector<std::string> wakeWords;

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
    VoiceRecognition transcribeModel;



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

    // Initilise Wakewords
    // Only threadsafe if only read in Transcript Thread
    for (auto ww : config.wakeWords) {
        wakeWords.push_back(ww);
        if (isVerbose) std::cout << "pushing wakeword: " << ww << std::endl;
    }

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
    if (config.audioEnable) {
        std::cout << "Initialising audio... ";
        for (auto &a : config.audio) {
            if (a.function == "playback") {
                if (isVerbose) std::cout << "Initilsing playback audio" << std::endl;
                ttsPlayback.setChannels(a.channels);
                ttsPlayback.setSampleRate(a.sampleRate);
                ttsPlayback.setIsVerbose(isVerbose);
            } else if (a.function == "inputStream") {
                if (isVerbose) std::cout << "Initilsing inputStream audio" << std::endl;
                recordVoice.setChannels(a.channels);
                recordVoice.setDuration(a.duration);
                recordVoice.setGain(a.gain);
                recordVoice.setSampleRate(a.sampleRate);
                recordVoice.setVADEnable(a.VADEnabled);
                recordVoice.setVAD(a.VADThresholdMult, a.VADSpeechTriggerFrames, a.VADSilenceTriggerFrames);
                recordVoice.setIsVerbose(isVerbose);
                for (auto &f : a.filters) {
                    if (isVerbose) std::cout << "Initilsing inputStream audio filters" << std::endl;
                    recordVoice.addFilter(f.type, f.cutoff, f.order);
                }
            }
        }

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

    // Initialise TTS voice synthesizer
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

    //Initilise Voice Recognition
    if (config.voiceRec.enabled) {
        std::cout << "Initilising Voice Transcript... ";
        transcribeModel.setFilePath(config.voiceRec.filePath);
        transcribeModel.setLanguage(config.voiceRec.language);
        transcribeModel.setMaxLen(config.voiceRec.max_len);
        transcribeModel.setNoContext(config.voiceRec.no_context);
        transcribeModel.setNoSpeechThreshold(config.voiceRec.no_speech_thold);
        transcribeModel.setNoTimestamps(config.voiceRec.no_timestamps);
        transcribeModel.setPrintProgress(config.voiceRec.print_progress);
        transcribeModel.setPrintRealtime(config.voiceRec.print_realtime);
        transcribeModel.setPrintSpecial(config.voiceRec.print_special);
        transcribeModel.setPrintTimestamps(config.voiceRec.print_timestamps);
        transcribeModel.setThreadCount(config.voiceRec.n_thread);
        transcribeModel.setSingleSegment(config.voiceRec.single_segment);
        transcribeModel.setTranslate(config.voiceRec.translate);
        transcribeModel.setUseGPU(config.voiceRec.use_gpu);
        try {
            transcribeModel.init();
        } catch (const std::exception &e) {
            std::cerr << "Error initialising Voice Transcript: " << e.what() << std::endl;
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


    // Thread loops
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
    std::thread streamVoiceThread([&recordVoice, &StreamAudio] {
        while(true) {
            recordVoice.streamAudio(10);
        }
    });
    std::thread captureVoiceThread([&recordVoice, &voiceRecording] {
        while(true) {
            {
                std::lock_guard<std::mutex> lock(trnscpt_mtx);
                auto data = recordVoice.moveStreamedAudioData();
                voiceRecording.push(data);
                trnscpt_cv.notify_one();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); //Ease up on CPU
        }
    });
    std::thread transcribeThread([&voiceRecording, &transcribeModel, &commands, isVerbose, &wakeWord, &wakeWords, &recordVoice] {
        std::vector<float> data;
        std::string transcript;
        std::unique_ptr<FunctionCall::ParsedPhrase> output;
        while(true) {
            {
                std::unique_lock<std::mutex> lock(trnscpt_mtx);
                trnscpt_cv.wait(lock, [&voiceRecording] { return !voiceRecording.empty(); });
                data = voiceRecording.front();
                voiceRecording.pop();
            }
            try {
                transcribeModel.transcribe(data);
            } catch (const std::exception &e) {
                std::cerr << "Error Transcribing Text: " << e.what() << std::endl;
                return 1;
            }
            transcript = transcribeModel.getTranscript();
            if (isVerbose) std::cout << "Transcribed text: " << transcript << std::endl;
            data.clear();
            bool ww = wakeWord.load();
            if (isVerbose) std::cout << "WakeWord: " << ww << std::endl;
            if (ww) {
                std::lock_guard<std::mutex> lock(command_mtx);
                commands.push(transcript);
                command_cv.notify_one();
            } else {
                for (auto &ww : wakeWords) {
                    if (transcript == ww) {
                        if(isVerbose) std::cout << "WakeWord called: " << ww << std::endl;
                        wakeWord.store(true);
                    }
                }
                if (isVerbose && !wakeWord) std::cout << "could not find wakeWord" << std::endl;
            }
        }
    });
    bool modelEnable = config.ModelEnable;
    std::thread textInputThread([&aRetry, modelEnable, &commands] {
        std::string input = "";
        while (true) {
            bool retry = aRetry.load();
            if (!retry || !modelEnable) {
                std::cout << "> ";
                getline(std::cin, input);
                {
                    std::lock_guard<std::mutex> lock(command_mtx);
                    commands.push(input);
                    command_cv.notify_one();
                }
                input = "";
            }
        }
    });

    std::cout << "Azazel Assistant v0.4 is running...\n";


    // Main loop
    while (true) {
        bool retry = aRetry.load();
        if (retry && config.ModelEnable) {
            // Retry with LLM
            if (isVerbose) std::cout << "Retrying with AI parsed command..." << std::endl;
            try {
                    input = commandModel.respond(userInput);
            } catch (const std::exception &e) {
                std::cerr << "Error generating command from AI: " << e.what() << std::endl;
                return 1;
            }
            if (isVerbose) std::cout << "AI parsed command: " << input << std::endl;
        }
        {
            std::unique_lock<std::mutex> lock(command_mtx);
            command_cv.wait(lock, [&commands] { return !commands.empty(); });

            input = commands.front();
            commands.pop();
        }
        if (userInput == "quit" || userInput == "q") break;
        // command parsing
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
            aRetry.store(false);
        }
        wakeWord.store(false);
        input = "";
        parsedPhrasePtr = nullptr;
        response = "";
    }
    ttsThread.join();
    streamVoiceThread.join();
    captureVoiceThread.join();
    transcribeThread.join();
    textInputThread.join();
    return 0;
}

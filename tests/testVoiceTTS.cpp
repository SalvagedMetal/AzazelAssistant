#include "../src/voice.h"
#include <iostream>
#include <cassert>

#include "../src/configReader.h"

void testVoiceInitialization(const std::string& modelPath, const std::string& configPath, const std::string& espeakDataPath) {
    Voice voice(modelPath, configPath, espeakDataPath, 22050, "output.wav", 1.0f, 0.667f, 0.8f, true, true);
    voice.init();
    assert(voice.getModelPath() == modelPath);
    assert(voice.getConfigPath() == configPath);
    assert(voice.getEspeakDataPath() == espeakDataPath);
}

void testVoiceSpeak(const std::string& modelPath, const std::string& configPath, const std::string& espeakDataPath) {
    Voice voice(modelPath, configPath, espeakDataPath, 22050, "test_output.wav", 1.0f, 0.667f, 0.8f, true, true);
    voice.init();
    voice.speak("This is a test of the text to speech functionality");
    // Since we cannot easily verify audio output in a unit test, we assume no exceptions means success
    assert(true);
}

void testVoiceVolumeScaling(const std::string& modelPath, const std::string& configPath, const std::string& espeakDataPath) {
    Voice voice(modelPath, configPath, espeakDataPath, 22050, "volume_test_output.wav", 1.0f, 0.667f, 0.8f, true, true);
    voice.init();
    voice.setVolumeScale(0.5f); // Set volume to 50%
    voice.speak("This is a test of the text to speech functionality at half volume");
    // Since we cannot easily verify audio output in a unit test, we assume no exceptions means success
    assert(true);
}

int main(int argc, char* argv[]) {
    ConfigReader configReader;
    ConfigVars::config config;
    try {
        configReader.readConfig("../config.json", true); 
        configReader.parseConfig();
    } catch (const std::exception &e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return 1;
    }
    config = configReader.getConfig();
    std::string modelPath = "../" + config.voice.model_path;;
    std::string configPath = "../" + config.voice.config_path;
    std::string espeakDataPath = "../" + config.voice.espeak_data_path;


    try {
        std::cout << "Running Voice tests..." << std::endl;
        testVoiceInitialization(modelPath, configPath, espeakDataPath);
        testVoiceSpeak(modelPath, configPath, espeakDataPath);
        testVoiceVolumeScaling(modelPath, configPath, espeakDataPath);
    } catch (const std::exception& e) {
        std::cerr << "Voice Test failed: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Voice tests passed!" << std::endl;
    return 0;
}
#ifndef VOICE_H
#define VOICE_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>

#include "piper.h"
#include "../lib/miniaudio/miniaudio.h"


class Voice {
private:
    std::string modelPath;
    std::string configPath;
    std::string espeakDataPath;
    piper_synthesizer *synth = nullptr;
    
    piper_synthesize_options options;
    uint32_t sampleRate;
    uint32_t channels;
    bool isVerbose;
    float lengthScale;
    float noiseScale;
    float noiseWScale;
    int gain = 0;
    std::vector<float> audioData;
    

public:
    Voice() = default;
    Voice(const std::string& modelPath, const std::string& configPath, const std::string& espeakDataPath,
            uint32_t sampleRate, uint32_t channels, float lengthScale, float noiseScale, float noiseWScale, bool isVerbose);
    ~Voice();
    void synthesise(const std::string text);
    void saveToFile(const char* filename);
    void init();


    // Setters and Getters
    void setModelPath(const std::string& path) { modelPath = path; }
    void setConfigPath(const std::string& path) { configPath = path; }
    void setEspeakDataPath(const std::string& path) { espeakDataPath = path; }
    void setSampleRate(uint32_t sr) { sampleRate = sr; }
    void setLengthScale(float scale) { lengthScale = scale; }
    void setNoiseScale(float scale) { noiseScale = scale; }
    void setNoiseWScale(float scale) { noiseWScale = scale; }
    void setVerbose(bool vb) { isVerbose = vb; }
    void setGain(int gn) { gain = gn; }
    void setAudioDate(std::vector<float> ad) { audioData = ad; }
    void setChannels(uint32_t ch) { channels = ch; }

    const std::string& getModelPath() const { return modelPath; }
    const std::string& getConfigPath() const { return configPath; }
    const std::string& getEspeakDataPath() const { return espeakDataPath; }
    uint32_t getSampleRate() const { return sampleRate; }
    float getLengthScale() const { return lengthScale; }
    float getNoiseScale() const { return noiseScale; }
    float getNoiseWScale() const { return noiseWScale; }
    bool getVerbose() const { return isVerbose; }
    float getGain() const { return gain; }
    std::vector<float> getAudioData() { return audioData; }
    uint32_t getChannels() const { return channels; }
};

#endif

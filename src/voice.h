#ifndef VOICE_H
#define VOICE_H

#include "piper.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>


class Voice {
private:
    std::string modelPath;
    std::string configPath;
    std::string espeakDataPath;
    piper_synthesizer *synth = nullptr;
    
    piper_synthesize_options options;
    std::ofstream audio_stream;
    std::string fileName;
    int frequency;
    bool isVerbose;
    bool enabled;
    float lengthScale;
    float noiseScale;
    float noiseWScale;
    float volumeScale = 1.0f;

public:
    Voice() = default;
    Voice(const std::string& modelPath, const std::string& configPath, const std::string& espeakDataPath,
            int freq, std::string fn, float lengthScale, float noiseScale, float noiseWScale, bool enabled, bool isVerbose);
    ~Voice();
    void speak(std::string text);
    void init();


    // Setters and Getters
    void setModelPath(const std::string& path) { modelPath = path; }
    void setConfigPath(const std::string& path) { configPath = path; }
    void setEspeakDataPath(const std::string& path) { espeakDataPath = path; }
    void setFileName(const std::string& fn) { fileName = fn; }
    void setFrequency(int freq) { frequency = freq; }
    void setLengthScale(float scale) { lengthScale = scale; }
    void setNoiseScale(float scale) { noiseScale = scale; }
    void setNoiseWScale(float scale) { noiseWScale = scale; }
    void setEnabled(bool en) { enabled = en; }
    void setVerbose(bool vb) { isVerbose = vb; }
    void setVolumeScale(float scale) { volumeScale = scale; }

    const std::string& getModelPath() const { return modelPath; }
    const std::string& getConfigPath() const { return configPath; }
    const std::string& getEspeakDataPath() const { return espeakDataPath; }
    const std::string& getFileName() const { return fileName; }
    int getFrequency() const { return frequency; }
    float getLengthScale() const { return lengthScale; }
    float getNoiseScale() const { return noiseScale; }
    float getNoiseWScale() const { return noiseWScale; }
    bool getEnabled() const { return enabled; }
    bool getVerbose() const { return isVerbose; }
    float getVolumeScale() const { return volumeScale; }
};

#endif

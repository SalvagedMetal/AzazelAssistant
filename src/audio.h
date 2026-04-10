#ifndef audio_h
#define audio_h

#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <thread>
#include <chrono>
#include <cstring>
#include <atomic>

#include "../lib/miniaudio/miniaudio.h"

struct AudioContext {
        ma_decoder* decoder;
        ma_audio_buffer* audioBuffer;
        std::atomic<float>* volume;
        ma_uint32 channels;
};

class InputAudio {
private:
    ma_uint32 sampleRate = 0;
    ma_uint32 channels = 0;
    float duration = 0;
    int gain = 0; // Gain in decibels, default is 0 (no gain)
    bool isVerbose = false;

    // Sound capture object variables
    ma_device_config deviceConfig;
    ma_device device;
    std::vector<float> audioBuffer;

public:
    InputAudio() = default;
    InputAudio(const ma_uint32 sampleRate, const ma_uint32 channels, const bool isVerbose);
    ~InputAudio() = default;
    void recordAudio(const float duration, const bool saveToFile, const char* filename);
    void init();

    //Getters and setters
    void setGain(const int gain);

    std::vector<float> getAudioBuffer();
    ma_device_config getdeviceConfig();
    ma_format getFormat();
    ma_uint32 getSampleRate();
    ma_uint32 getChannels();
    float getGain();

};



class OutputAudio {
private:
    ma_uint32 sampleRate = 0;
    ma_uint32 channels = 0;
    float duration = 0;
    float volumeScale = 1.0f; // Linear volume 1.0f is default
    bool isVerbose = false;

    // Sound capture object variables
    ma_device_config deviceConfig;
    ma_device device;
    std::vector<float> audioBuffer;

public:
    OutputAudio() = default;
    ~OutputAudio() = default;
    OutputAudio(const ma_uint32 sampleRate, const ma_uint32 channels, const bool isVerbose);
    void init();
    void playAudioFile(const char* filename);
    void playAudioBuffer(const std::vector<float> &samples);
    void playFromLastPosistion(); // TODO in tandem with outputaudio.cpp:168

    // Getters and Setters
    void setVolumeScale(const float volumeScale);

    ma_device_config getDeviceConfig();
    ma_format getFormat();
    ma_uint32 getSampleRate();
    ma_uint32 getChannels();
    float getVolumeScale();

};
#endif
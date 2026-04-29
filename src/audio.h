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

namespace Audio {
    enum Filter {
        NONE = 0,
        HPF,
        LPF,
        BANDPF
    };
};

class InputAudio {
private:
    ma_uint32 sampleRate = 0;
    ma_uint32 channels = 0;
    float duration = 0.0f;
    int gain = 0; // Gain in decibels, default is 0 (no gain)
    bool isVerbose = false;

    // struct for filtering
    struct filter {
        int type;
        double cutoff;
        ma_uint32 order;
    };
    filter filter = {0};

    // Sound capture object variables
    ma_device_config deviceConfig;
    ma_hpf_config hpFilterConfig;
    ma_lpf_config lpFilterConfig;
    ma_bpf_config bpFilterConfig;
    ma_device device;
    std::vector<float> audioBuffer;

    struct AudioContext {
        std::vector<float> audioBuffer;
        std::vector<float> tempBuffer;
        ma_lpf lpf;
        ma_hpf hpf;
        ma_bpf bpf;
        int type = 0;
        ma_uint32 channels;
    };
    AudioContext ctx;

public:
    InputAudio() = default;
    InputAudio(const ma_uint32 sampleRate, const ma_uint32 channels, const int filterType, const bool isVerbose);
    ~InputAudio() = default;
    void recordAudio(const float duration);
    void saveToFile(const char* filename);
    void init();

    // Getters and Setters
    void setGain(const int gn) { gain = gn; }
    void setDuration(const float dur) { duration = dur; }
    void setIsVerbose(const bool vb) { isVerbose = vb; }
    void setChannels(const ma_uint32 ch) { channels = ch; }
    void setSampleRate(const ma_uint32 sr) {sampleRate = sr; }
    void setAudioBuffer(const std::vector<float> ab) { audioBuffer = ab; } 
    void setFilterType(const int filterType) { filter.type = filterType; }
    void setFilterOrder(const ma_uint32 order) { filter.order = order; }
    void setFilterCutoff(const double cutoff) { filter.cutoff = cutoff; }

    std::vector<float> getAudioBuffer() { return audioBuffer; }
    ma_device_config getdeviceConfig() { return deviceConfig; }
    ma_format getFormat() { return deviceConfig.capture.format; }
    ma_uint32 getSampleRate() { return sampleRate; }
    ma_uint32 getChannels() { return channels; }
    float getGain() { return gain; }
    int getFilterTyoe() { return filter.type; }
    ma_uint32 getFilterOrder() { return filter.order; }
    double getFilterCutoff() { return filter.cutoff; }
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

    struct AudioContext {
        ma_decoder* decoder;
        ma_audio_buffer* audioBuffer;
        std::atomic<float>* volume;
        ma_uint32 channels;
    };
    AudioContext ctx;

public:
    OutputAudio() = default;
    ~OutputAudio() = default;
    OutputAudio(const ma_uint32 sampleRate, const ma_uint32 channels, const bool isVerbose);
    void init();
    void playAudioFile(const char* filename);
    void playAudioBuffer(const std::vector<float> &samples);
    void playFromLastPosistion(); // TODO in tandem with outputaudio.cpp:168

    // Getters and Setters
    void setVolumeScale(const float vs) { volumeScale = vs; }
    void boolIsVerbose(const bool vb) { isVerbose = vb; }
    void setChannels(const ma_uint32 ch) { channels = ch; }
    void setSampleRate(const ma_uint32 sr) {sampleRate = sr; }
    void setAudioBuffer(const std::vector<float> ab) { audioBuffer = ab; }
    void setIsVerbose(const bool vb) { isVerbose = vb; }

    std::vector<float> getAudioBuffer() { return audioBuffer; }
    ma_device_config getdeviceConfig() { return deviceConfig; }
    ma_format getFormat() { return deviceConfig.playback.format; }
    ma_uint32 getSampleRate() { return sampleRate; }
    ma_uint32 getChannels() { return channels; }
    float getVolumeScale() { return volumeScale; }

};
#endif
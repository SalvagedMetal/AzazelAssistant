#ifndef audio_h
#define audio_h

#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <atomic>
#include <utility>

#include "../lib/miniaudio/miniaudio.h"

namespace Audio {
    enum Filter {
        NONE = 0,
        HPF,
        LPF,
        BANDPF
    };
    // Goertzel DFT Algorithm, returns magnitude of traget frequency
    float goertzel(const std::vector<float>& samples, const float targetFreq, const float sampleRate);
};

class InputAudio {
private:
    ma_uint32 sampleRate = 0;
    ma_uint32 channels = 0;
    float duration = 0.0f;
    int gain = 0; // Gain in decibels, default is 0 (no gain)
    bool isVerbose = false;
    std::atomic<bool> aRunning = true;
    int targetSamples = 0;
    bool VADEnable = false;
    float VADThresholdMult = 3.5f;
    int VADSpeechTriggerFrames = 3;
    int VADSilenceTriggerFrames = 50;

    // struct for filtering
    struct Filter {
        Audio::Filter type;
        double cutoff;
        ma_uint32 order;
        union {
            ma_lpf lpf;
            ma_hpf hpf;
            ma_bpf bpf;
        };
    };

    // Sound capture object variables
    ma_device_config deviceConfig;
    ma_device device;
    std::vector<float> audioData;

    // Data to enter callback
    struct AudioContext {
        std::vector<float> audioBuffer, tempBuffer;
        ma_uint32 channels;
        ma_rb ringBuffer;
        std::vector<Filter> filters;
    };
    AudioContext ctx;

    // Co-ordination objects
    std::mutex data_mtx;
    std::condition_variable data_cv;
    bool dataReady = false;

public:
    InputAudio() = default;
    InputAudio(const ma_uint32 sampleRate, const ma_uint32 channels, const bool isVerbose);
    ~InputAudio() = default;
    void recordAudio(const float duration);
    void streamAudio(const float maxDuration);
    void saveToFile(const char* filename, const std::vector<float> &data);
    void init();
    void clearData();
    void addFilter(const Audio::Filter type, const double cutoff, const ma_uint32 order);

    // Getters and Setters
    void setGain(const int gn) { gain = gn; }
    void setDuration(const float dur) { duration = dur; }
    void setIsVerbose(const bool vb) { isVerbose = vb; }
    void setChannels(const ma_uint32 ch) { channels = ch; }
    void setSampleRate(const ma_uint32 sr) {sampleRate = sr; }
    void setAudioData(const std::vector<float> ad) { audioData = ad; } 
    void setRunning(const bool run) { aRunning.store(run); }
    void setVADEnable(const bool en) { VADEnable = en; }
    void setVAD(const float mult, const int speechTrig, const int silenceTrig); // Defined in inputAudio.cpp

    std::vector<float> getAudioData(); // defined in inputAudio.cpp
    std::vector<float> moveStreamedAudioData(); // defined in inputAudio.cpp
    ma_device_config getdeviceConfig() { return deviceConfig; }
    ma_format getFormat() { return deviceConfig.capture.format; }
    ma_uint32 getSampleRate() { return sampleRate; }
    ma_uint32 getChannels() { return channels; }
    float getGain() { return gain; }
    bool getRunning() { return aRunning.load(); }
    bool getVADEnable() { return VADEnable; }
    float getVADThresholdMult() { return VADThresholdMult; }
    int getVADSpeechTriggerFrames() { return VADSpeechTriggerFrames; }
    int getVADSilenceTriggerFrames() { return VADSilenceTriggerFrames; }

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

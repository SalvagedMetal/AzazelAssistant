#ifndef VOICEREC_H
#define VOICEREC_H

#include <iostream>
#include <string>
#include <vector>

#include "whisper.h"

class VoiceRecognition {
private:
    whisper_context_params cparams;
    whisper_full_params wparams;
    struct whisper_context *ctx;

    bool isVerbose = false;
    std::string fileName = "ggml-tiny.en.bin";
    std::string transcript = "";

    // Parameter variables
    bool use_gpu = false;
    float no_speech_thold = 0.3f;
    int max_len = 50;
    bool translate = false;
    int n_threads = 4;
    bool no_context = true;
    bool no_timestamps = true;
    bool single_segment = false;
    std::string language = "en";
    bool print_progress = false;
    bool print_realtime = false;
    bool print_special = false;
    bool print_timestamps = false;

public:
    VoiceRecognition() = default;
    ~VoiceRecognition() = default;

    void init();
    void transcribe(std::vector<float> &audioData);
    void clear();

    // Getters and Setters

    bool getVerbose() const { return isVerbose; }
    const std::string& getFileName() const { return fileName; }
    const std::string& getTranscript() const { return transcript; }
    bool getUseGPU() const { return use_gpu; }
    float getNoSpeechThreshold() const { return no_speech_thold; }
    int getMaxLen() const { return max_len; }
    bool getTranslate() const { return translate; }
    int getThreadCount() const { return n_threads; }
    bool getNoContext() const { return no_context; }
    bool getNoTimestamps() const { return no_timestamps; }
    bool getSingleSegment() const { return single_segment; }
    const std::string& getLanguage() const { return language; }
    bool getPrintProgress() const { return print_progress; }
    bool getPrintRealtime() const { return print_realtime; }
    bool getPrintSpecial() const { return print_special; }
    bool getPrintTimestamps() const { return print_timestamps; }

    void setVerbose(bool value) { isVerbose = value; }
    void setFileName(const std::string& value) { fileName = value; }
    void setTranscript(const std::string& value) { transcript = value; }
    void setUseGPU(bool value) { use_gpu = value; }
    void setNoSpeechThreshold(float value) { no_speech_thold = value; }
    void setMaxLen(int value) { max_len = value; }
    void setTranslate(bool value) { translate = value; }
    void setThreadCount(int value) { n_threads = value; }
    void setNoContext(bool value) { no_context = value; }
    void setNoTimestamps(bool value) { no_timestamps = value; }
    void setSingleSegment(bool value) { single_segment = value; }
    void setLanguage(const std::string& value) { language = value; }
    void setPrintProgress(bool value) { print_progress = value; }
    void setPrintRealtime(bool value) { print_realtime = value; }
    void setPrintSpecial(bool value) { print_special = value; }
    void setPrintTimestamps(bool value) { print_timestamps = value; }
};

#endif

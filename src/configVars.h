#ifndef CONFIGVARS_H
#define CONFIGVARS_H

#include "audio.h"

namespace ConfigVars {
    struct Model {
        std::string name;
        std::string purpose;
        std::string path;
        int ngl;
        int n_ctx;
        float temp;
        float min_p;
        float top_p;
        float typical;
        int top_k;
        float dist;
        std::string init_message;
        bool keepHistory;
    };

    struct MQTTCommand {
        std::string name;
        std::string type; // "publish" or "subscribe"
        std::string topic;
        int qos;
        bool retain; // only for publish
        std::string payload;
        int NArgs;
    };

    struct MQTTConfig {
        bool enabled;
        std::string broker_ip;
        int broker_port;
        int keepalive;
        std::string username;
        std::string password;
        std::string client_id;
        bool clean_session;
        std::vector<MQTTCommand> commands;
    };

    struct Commands {
        std::string name;
        std::string function;
        int NArgs;
        bool confirmation;
        int priority;
        std::vector<std::string> phrases;
    };

    struct VoiceConfig {
        bool enabled;
        std::string name;
        std::string model_path;
        std::string config_path;
        std::string espeak_data_path;
        int sample_rate;
        int gain;
        float length_scale;
        float noise_scale;
        float noise_w_scale;
    };

    struct AudioConfig {
        bool enabled;
        std::string function;
        uint32_t channels;
        uint32_t sampleRate;
        int gain;
        float duration;
        bool VADEnabled;
        float VADThresholdMult;
        int VADSpeechTriggerFrames;
        int VADSilenceTriggerFrames;
        std::vector<Audio::Filter> filters;
    };

    struct VoiceRecConfig {
        bool enabled;
        std::string filePath;
        std::string language;
        bool use_gpu;
        float no_speech_thold;
        int max_len;
        bool translate;
        int n_thread;
        bool no_context;
        bool no_timestamps;
        bool single_segment;
        bool print_progress;
        bool print_realtime;
        bool print_special;
        bool print_timestamps;
    };
    
    // Overall configuration structure
    struct config {
        bool ModelEnable;
        bool audioEnable;
        std::vector<Model> models;
        MQTTConfig mqtt;
        std::vector<Commands> commandCalls;
        VoiceConfig voice;
        std::vector<AudioConfig> audio;
        std::vector<std::string> wakeWords;
        VoiceRecConfig voiceRec;
    };
};


#endif

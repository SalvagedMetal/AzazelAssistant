#ifndef CONFIGVARS_H
#define CONFIGVARS_H

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
        std::string output_file;
        float length_scale;
        float noise_scale;
        float noise_w_scale;
    };
    
    // Overall configuration structure
    struct config {
        bool ModelEnable;
        std::vector<Model> models;
        MQTTConfig mqtt;
        std::vector<Commands> commandCalls;
        VoiceConfig voice;
    };
};


#endif

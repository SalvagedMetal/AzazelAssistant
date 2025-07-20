#ifndef MODEL_H
#define MODEL_H

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <memory>

#include "llama.h"

class Model {
    private:
    struct chat_messages {
        std::string role; // "system", "user", or "assistant"
        std::string content; // message content
    };

    // Deleters for llama model components
    struct LlamaModelDeleter {
        void operator()(llama_model* m) const {
            if (m) llama_model_free(m);
        }
    };
    struct LlamaContextDeleter {
        void operator()(llama_context* c) const {
            if (c) llama_free(c);
        }
    };
    struct LlamaSamplerDeleter {
        void operator()(llama_sampler* s) const {
            if (s) llama_sampler_free(s);
        }
    };

    std::string model_name = "Phi-4-mini-instruct-Q6_K_L";
    std::string model_purpose = "command execution";
    std::string model_path = "models/microsoft_Phi-4-mini-instruct-Q6_K_L.gguf";
    int ngl = 0; // default 0 for CPU only machines
    int n_ctx = 2048; // context size in tokens
    float temp = 0.1f;
    float min_p = 0.05f;
    float top_p = 0.95f;
    float typical = 0.95f;
    int top_k = 10;
    float dist = LLAMA_DEFAULT_SEED;
    std::string init_message =  "Always respond with a single valid JSON command from the following list: "
                                "{\"command\": \"chat\", \"arguments\": [\"<user prompt>\"], {\"command\": \"getCurrentDate\", \"arguments\": []} "
                                "Do not include any explanation. Only return one command object. "
                                "Valid Examples: User: What is the current time? Assistant: {\"command\": \"GetCurrentTime\", \"arguments\": []}, "
                                "User: What time is it? Assistant: {\"command\": \"GetCurrentTime\", \"arguments\": []}, "
                                "User: What is the current date? Assistant: {\"command\": \"GetCurrentDate\", \"arguments\": []}, "
                                "User: What date is it? Assistant: {\"command\": \"GetCurrentDate\", \"arguments\": []},"
                                "User: Hello, how are you? Assistant: {\"command\": \"chat\", \"arguments\": [\"Hello, how are you?\"]}, "
                                "User: Can you give me a recpie for a cake? Assistant: {\"command\": \"chat\", \"arguments\": [\"Can you give me a recpie for a cake?\"]}.";
    bool keepHistory = true; // whether to keep the chat history
    bool isVerbose = false; // whether to return verbose strings

    // Pointers to the model components
    std::unique_ptr<llama_model, LlamaModelDeleter> model;
    std::unique_ptr<llama_context, LlamaContextDeleter> ctx;
    std::unique_ptr<llama_sampler, LlamaSamplerDeleter> smpl;
    const llama_vocab *vocab = nullptr; // managed by llama.cpp

    std::vector<chat_messages> messages;

    public:
    Model(const Model&) = delete; // Copy constructor is not supported by llama.cpp
    Model& operator=(const Model&) = delete; // Copy operator is not supported by llama.cpp
    Model(Model&&) = default; // Move constructor
    Model& operator=(Model&&) = default; // Move assignment operator

    /*
    const std::string model_name    ||name of the model
    const std::string model_purpose ||purpose of the model
    const std::string model_path    ||path to the model file
    const int ngl                   ||number of GPU layers, 0 for CPU only
    const int n_ctx                 ||context size in tokens
    const std::string init_message  ||initial system message
    const float temp                ||temperature for sampling, 0.0 for deterministic output, 1.0 for maximum randomness
    const float min_p               ||minimum probability for sampling, 0.0 for no minimum, 0.05 for minimum probability
    const float top_p               ||top-p sampling parameter, 0.0 for no top-p, 0.95 for 95% of the probability mass
    const float typical             ||typical sampling parameter, 0.0 for no typical, 0.95 for typical sampling
    const float dist                ||distribution parameter for sampling, 0.0 for no distribution, LLAMA_DEFAULT_SEED for default distribution
    const int top_k                 ||top-k sampling parameter, choose only the most probable k tokens
    const bool keepHistory          ||whether to keep the chat history
    const bool isVerbose            ||return verbose strings */
    Model() = default;
    ~Model() = default;
    Model(const std::string&, const std::string&, const std::string&, const int, const int, 
          const std::string&, const float, const float, const float, const float, 
          const float, const int, const bool, const bool);
    
    void init();
    std::string generate(const std::string&);
    std::string respond(const std::string&);
    void clearHistory();

    // Getters
    std::string getModelName() const;
    std::string getModelPurpose() const;
    std::string getModelPath() const;
    std::string getInitMessage() const;
    float getTemp() const;
    float getMinP() const;
    float getTopP() const;
    float getTypical() const;
    float getDist() const;
    int getTopK() const;
    int getNGL() const;
    int getNCTX() const;
    int getKeepHistory() const;
    int getVerbose() const;
    std::vector<chat_messages> getMessages() const;

    // Setters
    void setModelName(const std::string&);
    void setModelPurpose(const std::string&);
    void setModelPath(const std::string&);
    void setNGL(const int);
    void setNCTX(const int);
    void setInitMessage(const std::string&);
    void setTemp(const float);
    void setMinP(const float);
    void setDist(const float);
    void setTopP(const float);
    void setTypical(const float);
    void setTopK(const int);
    void setKeepHistory(const bool);
    void setVerbose(const bool);
    void setMessages(const std::vector<chat_messages>& messages);
};

#endif
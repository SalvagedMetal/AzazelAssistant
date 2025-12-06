#include "model.h"

    // Initialize the model
    void Model::init() {
        
        // disables metadata dumping
        llama_log_set([](ggml_log_level, const char *, void *) {}, nullptr);

        // load dynamic backends
        ggml_backend_load_all();

        // initialize the model
        llama_model_params model_params = llama_model_default_params();
        model_params.n_gpu_layers = ngl;

        model.reset(llama_model_load_from_file(model_path.c_str(), model_params));
        if (!model) {
            throw std::runtime_error("Failed to load model");
        }
        vocab = llama_model_get_vocab(model.get());
        if (!vocab) {
            throw std::runtime_error("Failed to get vocabulary from the model");
        }

        // initialize the context
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = n_ctx;
        ctx_params.n_batch = n_ctx;

        ctx.reset(llama_init_from_model(model.get(), ctx_params));
        if (!ctx) {
            throw std::runtime_error("Failed to create context");
        }

        // add the initial system message
        messages.push_back({"system", init_message}); 

        // initialize the sampler
        smpl.reset(llama_sampler_chain_init(llama_sampler_chain_default_params()));
        llama_sampler_chain_add(smpl.get(), llama_sampler_init_min_p(min_p, 1));
        llama_sampler_chain_add(smpl.get(), llama_sampler_init_top_p(top_p, 1));
        llama_sampler_chain_add(smpl.get(), llama_sampler_init_typical(typical, 1));
        llama_sampler_chain_add(smpl.get(), llama_sampler_init_temp(temp));
        llama_sampler_chain_add(smpl.get(), llama_sampler_init_dist(dist));
        llama_sampler_chain_add(smpl.get(), llama_sampler_init_top_k(top_k));
        //llama_sampler_chain_add(smpl.get(), llama_sampler_init_mirostat(
        //    vocab->n_vocab, LLAMA_DEFAULT_SEED, 0.1f, 0.1f, 5));
        if (!smpl) {
            throw std::runtime_error("Failed to initialize the sampler");
        }
    }

    // Generate a response based on the prompt
    std::string Model::generate(const std::string &prompt) {
        std::string response;

        const bool is_first = llama_memory_seq_pos_max(llama_get_memory(ctx.get()), 0) == 0;

        // tokenize the prompt
        const int n_prompt_tokens = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, is_first, true);
        std::vector<llama_token> prompt_tokens(n_prompt_tokens);
        if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), is_first, true) < 0) {
            throw std::runtime_error("Failed to tokenize the prompt");
        }

        // prepare a batch for the prompt
        llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
        llama_token new_token_id;
            if (isVerbose) {
                std::cout << "Generated token: " << std::endl;
            }
            while (true) {
                // check if there is enough space in the context to evaluate this batch
                int n_ctx_batch = llama_n_ctx(ctx.get());
                int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx.get()), 0);
                if (n_ctx_used + batch.n_tokens > n_ctx_batch) {
                    throw std::runtime_error("Context size exceeded");
                    return "";
                }

                if (llama_decode(ctx.get(), batch)) {
                    throw std::runtime_error("Failed to decode");
                }

                // sample the next token
                new_token_id = llama_sampler_sample(smpl.get(), ctx.get(), -1);

                // is it an end of generation?
                if (llama_vocab_is_eog(vocab, new_token_id)) {
                    break;
                }

                // convert the token to a string, print it and add it to the response
                char buf[256];
                int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
                if (n < 0) {
                    throw std::runtime_error("Failed to convert token to piece");
                }
                std::string piece(buf, n);
                if (isVerbose) {
                    printf("%s", piece.c_str());
                }
                fflush(stdout);
                response += piece;

                // prepare the next batch with the sampled token
                batch = llama_batch_get_one(&new_token_id, 1);
            }
            if (isVerbose) {
                std::cout << std::endl;
            }

        return response;
    }

    std::string Model::respond(const std::string &prompt) {
        if (prompt.empty()) {
            return "";
        }
        // add the user input to the message list
        messages.push_back({"user", prompt});

        // format the messages for the chat template
        std::vector<llama_chat_message> llama_messages;
        for (const auto &msg : messages) {
            llama_messages.push_back({msg.role.c_str(), msg.content.c_str()});
        }
        if (!keepHistory) {
            if (messages.size() > 1) {
                messages.pop_back(); // remove the last user message if history is not kept
            }
        }

        std::vector<char> formatted(llama_n_ctx(ctx.get()));
        int prev_len = 0;
        std::string response = "";

        const char* tmpl = llama_model_chat_template(model.get(), /* name */ nullptr);

        // apply the chat template to format the messages
        int new_len = llama_chat_apply_template(tmpl, llama_messages.data(), llama_messages.size(), true, formatted.data(), formatted.size());
        if (new_len > (int)formatted.size()) {
            formatted.resize(new_len);
            new_len = llama_chat_apply_template(tmpl, llama_messages.data(), llama_messages.size(), true, formatted.data(), formatted.size());
        }
        if (new_len < 0) {
            throw std::runtime_error("Failed to apply the chat template");
            return "";
        }

        // remove previous messages to obtain the prompt to generate the response
        std::string formatted_prompt(formatted.begin() + prev_len, formatted.begin() + new_len);

        // generate a response
        response = generate(formatted_prompt);

        // add the response to the messages
        if (keepHistory) {
            messages.push_back({"assistant", response}); 
            std::vector<llama_chat_message> llama_messages_prev;
            for (const auto &msg : messages) {
                llama_messages_prev.push_back({msg.role.c_str(), msg.content.c_str()});
            }

            prev_len = llama_chat_apply_template(tmpl, llama_messages_prev.data(), llama_messages_prev.size(), false, nullptr, 0);
            if (prev_len < 0) {
                throw std::runtime_error("Failed to apply the chat template");
                return "";
            }
        }
        
        return response;
    }

    // Clear the model messages
    void Model::clearHistory() {
        while (messages.size() > 1) {
            messages.pop_back(); // keep the system message
        }
    }

    // Constructor with parameters
    Model::Model(const std::string& name, const std::string& purpose, const std::string& path, 
                 const int ngl, const int n_ctx, const std::string& init_msg, 
                 const float temp, const float min_p, const float top_p, 
                 const float typical, const float dist, const int top_k, const bool keep_history, const bool isVerbose)
        : model_name(name), model_purpose(purpose), model_path(path),
          ngl(ngl), n_ctx(n_ctx), init_message(init_msg),
          temp(temp), min_p(min_p), top_p(top_p), typical(typical),
          dist(dist), top_k(top_k), keepHistory(keep_history), isVerbose(isVerbose) {
    }

    

    void Model::setModelName(const std::string& input) { model_name = input; }
    void Model::setModelPurpose(const std::string& input) { model_purpose = input; }
    void Model::setModelPath(const std::string& input) { model_path = input; }
    void Model::setNGL(const int input) { ngl = input; }
    void Model::setNCTX(const int input) { n_ctx = input; }
    void Model::setInitMessage(const std::string& input) { init_message = input; }
    void Model::setTemp(const float input) { temp = input; }
    void Model::setMinP(const float input) { min_p = input; }
    void Model::setDist(const float input) { dist = input;}
    void Model::setTopP(const float input) { top_p = input; }
    void Model::setTypical(const float input) { typical = input; }
    void Model::setTopK(const int input) { top_k = input; }
    void Model::setKeepHistory(const bool input) { keepHistory = input; }
    void Model::setVerbose(const bool input) { isVerbose = input; }
    void Model::setMessages(const std::vector<chat_messages>& messages) { this->messages = messages; }

    std::string Model::getModelName() const { return model_name; }
    std::string Model::getModelPurpose() const { return model_purpose; }
    std::string Model::getModelPath() const { return model_path; }
    std::string Model::getInitMessage() const { return init_message; }
    float Model::getTemp() const { return temp; }
    float Model::getMinP() const { return min_p; }
    float Model::getTopP() const { return top_p; }
    float Model::getTypical() const { return typical; }
    float Model::getDist() const { return dist; }
    int Model::getTopK() const { return top_k; }
    int Model::getNGL() const { return ngl; }
    int Model::getNCTX() const { return n_ctx; }
    int Model::getKeepHistory() const { return keepHistory; }
    int Model::getVerbose() const { return isVerbose; }
    std::vector<Model::chat_messages> Model::getMessages() const { return messages; }
    
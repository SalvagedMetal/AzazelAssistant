#include "audio.h"

InputAudio::InputAudio(const ma_uint32 sampleRate, const ma_uint32 channels, const bool isVerbose) :
    sampleRate(sampleRate), channels(channels), isVerbose(isVerbose) {}


void InputAudio::init() {
    if (isVerbose) std::cout << "Starting Input Audio Initilisation" << std::endl;
    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = channels;
    deviceConfig.sampleRate = sampleRate;

    ctx.channels = channels;
    deviceConfig.pUserData = &ctx;
}


void InputAudio::recordAudio(const float duration) {
    // Initilise callback
    deviceConfig.dataCallback = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        auto *ctx = (AudioContext*)pDevice->pUserData;
        ctx->tempBuffer.resize(frameCount * ctx->channels);
        const float *input = (const float*)pInput; // so that previous filters don't overwrite eachother
        
        if (ctx->filters.empty()) {
            std::memcpy(ctx->tempBuffer.data(), input, (frameCount * ctx->channels) * sizeof(float));
        } else {
            for (auto& f : ctx->filters) {
                switch(f.type) {
                    case Audio::FilterType::HPF:
                        if (ma_hpf_process_pcm_frames(&f.hpf, ctx->tempBuffer.data(), input, frameCount) != MA_SUCCESS) {
                            std::cout << "Failure processing High Pass Filter frames" << std::endl;
                        }
                    break;
                    case Audio::FilterType::LPF:
                        if (ma_lpf_process_pcm_frames(&f.lpf, ctx->tempBuffer.data(), input, frameCount) != MA_SUCCESS) {
                            std::cout << "Failure processing Low Pass Filter frames" << std::endl;
                        }
                    break;
                    case Audio::FilterType::BANDPF:
                        if (ma_bpf_process_pcm_frames(&f.bpf, ctx->tempBuffer.data(), input, frameCount) != MA_SUCCESS) {
                            std::cout << "Failure processing Band Pass Filter frames" << std::endl;
                        }
                    break;
                }
                input = ctx->tempBuffer.data();
            }
        }

        ctx->audioBuffer.insert(ctx->audioBuffer.end(), ctx->tempBuffer.begin(), ctx->tempBuffer.begin() + (frameCount * ctx->channels));

        (void)pOutput;
    };

    ma_hpf_config hpFilterConfig;
    ma_lpf_config lpFilterConfig;
    ma_bpf_config bpFilterConfig;

    // Initilise filters
    if (isVerbose) std::cout << "Initilising Filters" << std::endl;
    for (auto& f : ctx.filters) {
        switch (f.type) {
            case Audio::FilterType::HPF:
                hpFilterConfig = ma_hpf_config_init(ma_format_f32, channels, sampleRate, f.cutoff, f.order);
                if (ma_hpf_init(&hpFilterConfig, nullptr, &f.hpf) != MA_SUCCESS) {
                    throw std::runtime_error("Failed to initialise High Pass Filter.");
                }
            break;
            case Audio::FilterType::LPF:
                lpFilterConfig = ma_lpf_config_init(ma_format_f32, channels, sampleRate, f.cutoff, f.order);
                if (ma_lpf_init(&lpFilterConfig, nullptr, &f.lpf) != MA_SUCCESS) {
                    throw std::runtime_error("Failed to initialise Low Pass Filter.");
                }
            break;
            case Audio::FilterType::BANDPF:
                bpFilterConfig = ma_bpf_config_init(ma_format_f32, channels, sampleRate, f.cutoff, f.order);
                if (ma_bpf_init(&bpFilterConfig, nullptr, &f.bpf) != MA_SUCCESS) {
                    throw std::runtime_error("Failed to initialise Low Pass Filter.");
                }
            break;
            default:
                f.type = Audio::FilterType::NONE;
            break;
        }
    }

    ctx.audioBuffer.clear();
    ctx.tempBuffer.clear();
    // preallocate memory before being used in audio thread
    ctx.audioBuffer.reserve(sampleRate * duration * channels);
    ctx.tempBuffer.reserve(4096 * channels);

    if (isVerbose) std::cout << "Starting Audio Recording" << std::endl;
     if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialise audio device.");
    }
    if (ma_device_start(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to start audio device.");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds((int)(duration * 1000)));
    if (ma_device_stop(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to stop audio device.");
    }
    ma_device_uninit(&device);
    
    // Uninitilise filters
    for (auto& f : ctx.filters) {
        switch (f.type) {
            case Audio::FilterType::HPF:
                ma_hpf_uninit(&f.hpf, nullptr);
            break;
            case Audio::FilterType::LPF:
                ma_lpf_uninit(&f.lpf, nullptr);
            break;
            case Audio::FilterType::BANDPF:
                ma_bpf_uninit(&f.bpf, nullptr);
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(data_mtx);
        audioData = ctx.audioBuffer;
        dataReady = true;
    }
    data_cv.notify_all();

    if (isVerbose) std::cout << "Gain setting at " << gain << "dB" << std::endl;
    // Gain control
    if (gain != 0) {
        float gainFactor = pow(10.0f, gain / 20.0f); // Convert dB to linear
        for (auto& sample : audioData) {
            sample *= gainFactor;
            sample = std::max(-1.0f, std::min(1.0f, sample)); // [-1.0, 1.0]
        }
    }
    deviceConfig.dataCallback = nullptr;
}

void InputAudio::saveToFile(const char* filename, const std::vector<float> &data) {
    std::lock_guard<std::mutex> lock(data_mtx);
    if (isVerbose) std::cout << "Saving to file " << filename << std::endl;
    ma_encoder encoder;
    ma_encoder_config encoderConfig;
    const char* lFilename;

    
    if (filename == nullptr || std::strlen(filename) == 0) {
        lFilename = "recorded_audio.wav"; // Default filename
    } else {
        lFilename = filename;
    }
    encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, channels, sampleRate);

    if (ma_encoder_init_file(lFilename, &encoderConfig, &encoder) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize encoder.");
    }
    if (data.empty()) {
        if (isVerbose) std::cout << "audioData is empty" << std::endl;
        ma_encoder_write_pcm_frames(&encoder, 0, 1, NULL);
    } else {
        ma_encoder_write_pcm_frames(&encoder, data.data(), data.size(), NULL);
    }

    ma_encoder_uninit(&encoder);
    lFilename = nullptr;
}


void InputAudio::streamAudio(const float maxDuration) {
    if (isVerbose) std::cout << "Streaming Audio" << std::endl;
    // Initilise Callback
    deviceConfig.dataCallback = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        auto *ctx = (AudioContext*)pDevice->pUserData;
        // allocate size
        size_t byteSize = frameCount * ctx->channels * sizeof(float);
        ctx->tempBuffer.resize(frameCount * ctx->channels);
        void *pWrite = nullptr;
        const float *input = (const float*)pInput; // so that previous filters don't overwrite eachother

        if (ctx->filters.empty()) {
            std::memcpy(ctx->tempBuffer.data(), input, (frameCount * ctx->channels) * sizeof(float));
        } else {
            for (auto& f : ctx->filters) {
                switch(f.type) {
                    case Audio::FilterType::HPF:
                        if (ma_hpf_process_pcm_frames(&f.hpf, ctx->tempBuffer.data(), input, frameCount) != MA_SUCCESS) {
                            std::cout << "Failure processing High Pass Filter frames" << std::endl;
                        }
                    break;
                    case Audio::FilterType::LPF:
                        if (ma_lpf_process_pcm_frames(&f.lpf, ctx->tempBuffer.data(), input, frameCount) != MA_SUCCESS) {
                            std::cout << "Failure processing Low Pass Filter frames" << std::endl;
                        }
                    break;
                    case Audio::FilterType::BANDPF:
                        if (ma_bpf_process_pcm_frames(&f.bpf, ctx->tempBuffer.data(), input, frameCount) != MA_SUCCESS) {
                            std::cout << "Failure processing Band Pass Filter frames" << std::endl;
                        }
                    break;
                }
                input = ctx->tempBuffer.data();
            }
        }
        
        ma_rb_acquire_write(&ctx->ringBuffer, &byteSize, &pWrite);
        if (byteSize > 0) {
            std::memcpy(pWrite, ctx->tempBuffer.data(), byteSize);
        }
        ma_rb_commit_write(&ctx->ringBuffer, byteSize);

        (void)pOutput;
    };

    ma_hpf_config hpFilterConfig;
    ma_lpf_config lpFilterConfig;
    ma_bpf_config bpFilterConfig;

    // Initilise filters
    if (isVerbose) std::cout << "Initilising Filters" << std::endl;
    for (auto& f : ctx.filters) {
        switch (f.type) {
            case Audio::FilterType::HPF:
                hpFilterConfig = ma_hpf_config_init(ma_format_f32, channels, sampleRate, f.cutoff, f.order);
                if (ma_hpf_init(&hpFilterConfig, nullptr, &f.hpf) != MA_SUCCESS) {
                    throw std::runtime_error("Failed to initialise High Pass Filter.");
                }
            break;
            case Audio::FilterType::LPF:
                lpFilterConfig = ma_lpf_config_init(ma_format_f32, channels, sampleRate, f.cutoff, f.order);
                if (ma_lpf_init(&lpFilterConfig, nullptr, &f.lpf) != MA_SUCCESS) {
                    throw std::runtime_error("Failed to initialise Low Pass Filter.");
                }
            break;
            case Audio::FilterType::BANDPF:
                bpFilterConfig = ma_bpf_config_init(ma_format_f32, channels, sampleRate, f.cutoff, f.order);
                if (ma_bpf_init(&bpFilterConfig, nullptr, &f.bpf) != MA_SUCCESS) {
                    throw std::runtime_error("Failed to initialise Low Pass Filter.");
                }
            break;
            default:
                f.type = Audio::FilterType::NONE;
            break;
        }
    }

    void *pRead = nullptr;
    size_t readBytes = 0;
    size_t requestedBytes = 0;
    if (VADEnable) {
        requestedBytes = sampleRate * channels * 0.03 * sizeof(float);
    } else {
       requestedBytes = sampleRate * channels * maxDuration * sizeof(float);

    }
    targetSamples = sampleRate * channels * maxDuration;
    
    if (isVerbose) std::cout << "Initilsing ring buffer" << std::endl;
    if (ma_rb_init(sampleRate * channels * maxDuration * sizeof(float), nullptr, nullptr, &ctx.ringBuffer) != MA_SUCCESS) {
        throw std::runtime_error("Failed to start ring buffer.");
    }

    if (isVerbose) std::cout << "Starting Audio Streaming" << std::endl;
    if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialise audio device.");
    }
    if (ma_device_start(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to start audio device.");
    }

    float noiseFloor = 0.0001f;
    bool speech = false;
    int speechFrames = 0;
    int silenceFrames = 0;
    int sampleEnergy = 0;

    while(aRunning.load()) {
        readBytes = requestedBytes;
        pRead = nullptr;

        if (ma_rb_acquire_read(&ctx.ringBuffer, &readBytes, &pRead) != MA_SUCCESS) {
            throw std::runtime_error("Failed to read ring buffer");
        }
        // skip until data is given
        if (readBytes == 0 || pRead == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        float *samples = (float*)pRead;
        int sampleCount = readBytes / sizeof(float);
        if (VADEnable) {
            float energy = 0.0f;

            for (int i = 0; i < sampleCount; i++) {
                float s = samples[i];
                energy += s * s;
            }
            energy /= sampleCount;

            // adaptive noise floor so that background noise gets filtered out
            noiseFloor = (0.95f * noiseFloor) + (0.05f * energy);

            float threshold = noiseFloor * VADThresholdMult;

            bool detected = energy > threshold;
            if ((sampleEnergy % 10) == 0) {
                if (isVerbose) std::cout << "energy: " << energy << std::endl;
            }
            if (detected) {
                speechFrames++;
                silenceFrames = 0;
                if (isVerbose) std::cout << "Speech detected" << std::endl;
            } else {
                silenceFrames++;
                speechFrames = 0;
            }
            sampleEnergy++;

            // Speech start
            if (!speech && speechFrames >= VADSpeechTriggerFrames) {
                speech = true;
                if (isVerbose) std::cout << "Speech started\n";
            }
            // Collect speech
            if (speech) {
                std::lock_guard<std::mutex> lock(data_mtx);
                audioData.insert(audioData.end(), samples, samples + sampleCount);
            }
            // Speech end
            if (speech && silenceFrames >= VADSilenceTriggerFrames) {
                speech = false;
                dataReady = true;
                data_cv.notify_all();
                if (isVerbose) std::cout << "Speech ended\n";
            }
        } else {
            {
                std::lock_guard<std::mutex> lock(data_mtx);
                audioData.insert(audioData.end(), samples, samples + sampleCount);
            }
                if ((audioData.size() >= targetSamples) && !dataReady) {
                dataReady = true;
                data_cv.notify_all();
                if (isVerbose) std::cout << "Speech saved\n";
            }
        }
        if (ma_rb_commit_read(&ctx.ringBuffer, readBytes) != MA_SUCCESS ) {
            throw std::runtime_error("Failed to commit ring buffer read");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    pRead = nullptr;

    if (ma_device_stop(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to stop audio device.");
    }
    ma_device_uninit(&device);

    // Uninitilise filters
    for (auto& f : ctx.filters) {
        switch (f.type) {
            case Audio::FilterType::HPF:
                ma_hpf_uninit(&f.hpf, nullptr);
            break;
            case Audio::FilterType::LPF:
                ma_lpf_uninit(&f.lpf, nullptr);
            break;
            case Audio::FilterType::BANDPF:
                ma_bpf_uninit(&f.bpf, nullptr);
            break;
        }
    }
    ma_rb_uninit(&ctx.ringBuffer);
    deviceConfig.dataCallback = nullptr;
}


std::vector<float> InputAudio::moveStreamedAudioData() {
    std::unique_lock<std::mutex> lock(data_mtx);

    data_cv.wait(lock, [&] { return (audioData.size() >= targetSamples) || dataReady; });
    dataReady = false;

    return std::move(audioData);
}


std::vector<float> InputAudio::moveAudioData() {
    std::unique_lock<std::mutex> lock(data_mtx);

    data_cv.wait(lock, [&] { return dataReady && !audioData.empty(); });
    dataReady = false;

    return std::move(audioData);
}


void InputAudio::clearData() {
    std::lock_guard<std::mutex> lock(data_mtx);

    audioData.clear();
    dataReady = false;
}


void InputAudio::addFilter(const Audio::FilterType type, const double cutoff, const ma_uint32 order) {
    Audio::Filter tempFilter;
    tempFilter.type = type;
    tempFilter.cutoff = cutoff;
    tempFilter.order = order;
    ctx.filters.push_back(tempFilter);
}


float Audio::goertzel(const std::vector<float>& samples, const float targetFreq, const float sampleRate) {
    int N = samples.size();
    int k = (int) (0.5 + ((N * targetFreq) / sampleRate));
    float w = ((2.0 * M_PI) / N) * k;
    float cosine = cos(w);
    float coeff = 2.0 * cosine;

    float q0 = 0, q1 = 0, q2 = 0;

    for (float sample : samples) {
        q0 = coeff * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;
    }

    return q1 * q1 + q2 * q2 - coeff * q1 * q2;
}


void InputAudio::setVAD(const float mult, const int speechTrig, const int silenceTrig) {
    VADThresholdMult = mult;
    VADSilenceTriggerFrames = silenceTrig;
    VADSpeechTriggerFrames = speechTrig;
}


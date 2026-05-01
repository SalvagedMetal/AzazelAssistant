#include "audio.h"

InputAudio::InputAudio(const ma_uint32 sampleRate, const ma_uint32 channels, const int filterType, const bool isVerbose) :
    sampleRate(sampleRate), channels(channels), isVerbose(isVerbose) {
        filter.type = filterType;
    }

void InputAudio::init() {
    if (isVerbose) std::cout << "Starting Input Audio Initilisation" << std::endl;
    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = channels;
    deviceConfig.sampleRate = sampleRate;

    switch (filter.type) {
        case Audio::Filter::HPF:
            hpFilterConfig = ma_hpf_config_init(ma_format_f32, channels, sampleRate, filter.cutoff, filter.order);
            ctx.type = Audio::Filter::HPF;
        break;
        case Audio::Filter::LPF:
            lpFilterConfig = ma_lpf_config_init(ma_format_f32, channels, sampleRate, filter.cutoff, filter.order);
            ctx.type = Audio::Filter::LPF;
        break;
        case Audio::Filter::BANDPF:
            bpFilterConfig = ma_bpf_config_init(ma_format_f32, channels, sampleRate, filter.cutoff, filter.order);
            ctx.type = Audio::Filter::BANDPF;
        break;
        default:
            ctx.type = Audio::Filter::NONE;
        break;
    }
    ctx.channels = channels;
    deviceConfig.pUserData = &ctx;
}


void InputAudio::recordAudio(const float duration) {
    // Initilise callback
    deviceConfig.dataCallback = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        auto *ctx = (AudioContext*)pDevice->pUserData;

        // Make sure buffer is big enough
        ctx->tempBuffer.resize(frameCount * ctx->channels);
        
        switch(ctx->type) {
            case Audio::Filter::HPF:
                if (ma_hpf_process_pcm_frames(&ctx->hpf, ctx->tempBuffer.data(), pInput, frameCount) != MA_SUCCESS) {
                    std::cout << "Failure processing High Pass Filter frames" << std::endl;
                }
            break;
            case Audio::Filter::LPF:
                if (ma_lpf_process_pcm_frames(&ctx->lpf, ctx->tempBuffer.data(), pInput, frameCount) != MA_SUCCESS) {
                    std::cout << "Failure processing Low Pass Filter frames" << std::endl;
                }
            break;
            case Audio::Filter::BANDPF:
                if (ma_bpf_process_pcm_frames(&ctx->bpf, ctx->tempBuffer.data(), pInput, frameCount) != MA_SUCCESS) {
                    std::cout << "Failure processing Band Pass Filter frames" << std::endl;
                }
            break;
            default:
                std::memcpy(ctx->tempBuffer.data(), pInput, (frameCount * ctx->channels) * sizeof(float));
            break;
        }

        ctx->audioBuffer.insert(ctx->audioBuffer.end(), ctx->tempBuffer.begin(), ctx->tempBuffer.begin() + (frameCount * ctx->channels));

        (void)pOutput;
    };

    // Initilise filters
    switch (filter.type) {
        case Audio::Filter::HPF:
            if (ma_hpf_init(&hpFilterConfig, nullptr, &ctx.hpf) != MA_SUCCESS) {
                throw std::runtime_error("Failed to initialise High Pass Filter.");
            }
        break;
        case Audio::Filter::LPF:
            if (ma_lpf_init(&lpFilterConfig, nullptr, &ctx.lpf) != MA_SUCCESS) {
                throw std::runtime_error("Failed to initialise Low Pass Filter.");
            }
        break;
        case Audio::Filter::BANDPF:
            if (ma_bpf_init(&bpFilterConfig, nullptr, &ctx.bpf) != MA_SUCCESS) {
                throw std::runtime_error("Failed to initialise Low Pass Filter.");
            }
        default:
            ctx.type = Audio::Filter::NONE;
        break;
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
    switch (filter.type) {
        case Audio::Filter::HPF:
            ma_hpf_uninit(&ctx.hpf, nullptr);
        break;
        case Audio::Filter::LPF:
            ma_lpf_uninit(&ctx.lpf, nullptr);
        break;
        case Audio::Filter::BANDPF:
            ma_bpf_uninit(&ctx.bpf, nullptr);
        break;
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
        void *pWrite = nullptr;
        // Make sure buffer is big enough
        ctx->tempBuffer.resize(frameCount * ctx->channels);

        switch(ctx->type) {
            case Audio::Filter::HPF:
                if (ma_hpf_process_pcm_frames(&ctx->hpf, ctx->tempBuffer.data(), pInput, frameCount) != MA_SUCCESS) {
                    std::cout << "Failure processing High Pass Filter frames" << std::endl;
                }
            break;
            case Audio::Filter::LPF:
                if (ma_lpf_process_pcm_frames(&ctx->lpf, ctx->tempBuffer.data(), pInput, frameCount) != MA_SUCCESS) {
                    std::cout << "Failure processing Low Pass Filter frames" << std::endl;
                }
            break;
            case Audio::Filter::BANDPF:
                if (ma_bpf_process_pcm_frames(&ctx->bpf, ctx->tempBuffer.data(), pInput, frameCount) != MA_SUCCESS) {
                    std::cout << "Failure processing Band Pass Filter frames" << std::endl;
                }
            break;
            default:
                std::memcpy(ctx->tempBuffer.data(), pInput, (frameCount * ctx->channels) * sizeof(float));
            break;
        }
        
        ma_rb_acquire_write(&ctx->ringBuffer, &byteSize, &pWrite);
        if (byteSize > 0) {
            std::memcpy(pWrite, ctx->tempBuffer.data(), byteSize);
        }
        ma_rb_commit_write(&ctx->ringBuffer, byteSize);

        (void)pOutput;
    };

    void *pRead = nullptr;
    size_t readBytes = 0;
    size_t requestedBytes = sampleRate * channels * maxDuration * sizeof(float);
    targetSamples = sampleRate * channels * maxDuration;

    // Initilise filters
    if (isVerbose) std::cout << "Initilising Filters" << std::endl;
    switch (filter.type) {
        case Audio::Filter::HPF:
            if (ma_hpf_init(&hpFilterConfig, nullptr, &ctx.hpf) != MA_SUCCESS) {
                throw std::runtime_error("Failed to initialise High Pass Filter.");
            }
        break;
        case Audio::Filter::LPF:
            if (ma_lpf_init(&lpFilterConfig, nullptr, &ctx.lpf) != MA_SUCCESS) {
                throw std::runtime_error("Failed to initialise Low Pass Filter.");
            }
        break;
        case Audio::Filter::BANDPF:
            if (ma_bpf_init(&bpFilterConfig, nullptr, &ctx.bpf) != MA_SUCCESS) {
                throw std::runtime_error("Failed to initialise Low Pass Filter.");
            }
        default:
            ctx.type = Audio::Filter::NONE;
        break;
    }

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

    while(running) {
        readBytes = requestedBytes;
       if (readBytes > 0) {
            if (ma_rb_acquire_read(&ctx.ringBuffer, &readBytes, &pRead) != MA_SUCCESS) {
                throw std::runtime_error("Failed to read ring buffer");
            }

            float *samples = (float*)pRead;
            int sampleCount = readBytes / (sizeof(float) * channels);
            {
                std::lock_guard<std::mutex> lock(data_mtx);
                audioData.insert(audioData.end(), samples, samples + sampleCount);
            }
            if ((audioData.size() >= targetSamples) && !dataReady) {
                dataReady = true;
                data_cv.notify_all();
            }
            if (ma_rb_commit_read(&ctx.ringBuffer, readBytes) != MA_SUCCESS ) {
                throw std::runtime_error("Failed to commit ring buffer read");
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (ma_device_stop(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to stop audio device.");
    }
    ma_device_uninit(&device);

    // Uninitilise filters
    switch (filter.type) {
        case Audio::Filter::HPF:
            ma_hpf_uninit(&ctx.hpf, nullptr);
        break;
        case Audio::Filter::LPF:
            ma_lpf_uninit(&ctx.lpf, nullptr);
        break;
        case Audio::Filter::BANDPF:
            ma_bpf_uninit(&ctx.bpf, nullptr);
        break;
    }
    ma_rb_uninit(&ctx.ringBuffer);
    deviceConfig.dataCallback = nullptr;
}


std::vector<float> InputAudio::moveStreamedAudioData() {
    std::unique_lock<std::mutex> lock(data_mtx);

    data_cv.wait(lock, [&] { return audioData.size() >= targetSamples; });
    dataReady = false;

    return std::move(audioData);
}


std::vector<float> InputAudio::getAudioData() {
    std::unique_lock<std::mutex> lock(data_mtx);

    data_cv.wait(lock, [&] { return dataReady && !audioData.empty(); });
    dataReady = false;

    return audioData;
}


void InputAudio::clearData() {
    std::lock_guard<std::mutex> lock(data_mtx);

    audioData.clear();
    dataReady = false;
}

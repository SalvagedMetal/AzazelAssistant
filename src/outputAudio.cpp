#include "audio.h"

OutputAudio::OutputAudio(const ma_uint32 sampleRate, const ma_uint32 channels, const bool isVerbose) :
    sampleRate(sampleRate), channels(channels), isVerbose(isVerbose) {}

void OutputAudio::init() {
    if (isVerbose) {
        std::cout << "Starting Ouptut Audio Initilisation" << std::endl;
    }
    // Set up device configuration based on whether it's for input (recording) or output (playback)
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.playback.channels = channels;
    deviceConfig.sampleRate        = sampleRate;
}


// TODO: add different audio formats, currently only wav is supported
void OutputAudio::playAudioFile(const char* filename) {
    if (isVerbose) {
        std::cout << "Starting PlayAudioFile" << std::endl;
    }
    bool errorFlag = false;
    ma_decoder decoder = {0};
    ma_decoder_config decoderConfig;
    std::atomic<float> aVolume(volumeScale);
    
    // Sends all required external data into dataCallback
    AudioContext ctx;
    ctx.decoder = &decoder;
    ctx.volume = &aVolume;
    deviceConfig.pUserData = &ctx;

    deviceConfig.dataCallback = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        auto* ctx = (AudioContext*)pDevice->pUserData;
        if (!ctx->decoder) {
            return;
        }

        ma_uint64 framesRead = 0;
        if (ma_decoder_read_pcm_frames(ctx->decoder, pOutput, frameCount, &framesRead) != MA_SUCCESS) {
            std::memset(pOutput, 0, frameCount * sizeof(float) * ctx->decoder->outputChannels);
        } else if (framesRead < frameCount) { // zero load the remaining frames
            std::memset((float*)pOutput + framesRead * ctx->decoder->outputChannels, 0,
            (frameCount - framesRead) * sizeof(float) * ctx->decoder->outputChannels);
        }

        float volume = ctx->volume->load();
        float* sampleOut = (float*)pOutput;
        if (volume != 1.0f) {
           for(ma_uint32 i = 0; i < (frameCount * ctx->decoder->outputChannels); i++) {
                sampleOut[i] *= volume;
                sampleOut[i] = std::max(-1.0f, std::min(1.0f, sampleOut[i])); // [-1.0, 1.0]
            }
        }
        (void)pInput;
    };

    decoderConfig = ma_decoder_config_init(ma_format_f32, channels, sampleRate);    
    if (ma_decoder_init_file(filename, &decoderConfig, &decoder) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize decoder.");
    }

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize audio device.");
    }

    // Check how long the audio file is and adjust the duration accordingly
    ma_uint64 framesTotal = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &framesTotal) != MA_SUCCESS) {
        throw std::runtime_error("Failed to get audio file length.");
    }
    if (ma_device_start(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to start audio device.");
    }

    // Wait until the audio has finished playing
    // TODO: add premature stop functionality and save cursor location (maybe use atomic fetch add/subtract)
    while (true) {
        ma_uint64 cursor = 0;
        if (ma_decoder_get_cursor_in_pcm_frames(&decoder, &cursor) != MA_SUCCESS) {
            cursor = MA_UINT64_MAX;
            errorFlag = true;
        }

        if (cursor >= framesTotal) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (errorFlag) {
        throw std::runtime_error("Failed to load cursor.");
    }
    if (ma_device_stop(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to stop audio device.");
    }
    ma_decoder_uninit(&decoder);
    deviceConfig.pUserData = nullptr;
    deviceConfig.dataCallback = nullptr;
    ma_device_uninit(&device);
}


void OutputAudio::playAudioBuffer(const std::vector<float> &samples) {
    if (isVerbose) {
        std::cout << "Starting PlayAudioBuffer" << std::endl;
    }
    bool errorFlag = false;
    ma_audio_buffer_config bufferConfig;
    ma_audio_buffer buffer = {0};
    std::atomic<float> aVolume(volumeScale);
    
    // Sends all required external data into dataCallback
    AudioContext ctx;
    ctx.audioBuffer = &buffer;
    ctx.volume = &aVolume;
    ctx.channels = channels;

    deviceConfig.sampleRate = sampleRate;
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = channels;
    deviceConfig.pUserData = &ctx;

    deviceConfig.dataCallback = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        auto* ctx = (AudioContext*)pDevice->pUserData;
        if (!ctx->audioBuffer) {
            return;
        }

        ma_uint64 framesRead = ma_audio_buffer_read_pcm_frames(ctx->audioBuffer, pOutput, frameCount, false);
        if (!framesRead) {
            std::memset(pOutput, 0, frameCount * sizeof(float) * ctx->channels);
        } else if (framesRead < frameCount) { // zero load the remaining frames
            std::memset((float*)pOutput + framesRead * ctx->channels, 0,
            (frameCount - framesRead) * sizeof(float) * ctx->channels);
        }

        float volume = ctx->volume->load();
        float* sampleOut = (float*)pOutput;
        if (volume != 1.0f) {
           for(ma_uint32 i = 0; i < (frameCount * ctx->channels); i++) {
                sampleOut[i] *= volume;
                sampleOut[i] = std::max(-1.0f, std::min(1.0f, sampleOut[i])); // [-1.0, 1.0]
            }
        }
        (void)pInput;
    };

    bufferConfig = ma_audio_buffer_config_init(ma_format_f32, channels, samples.size() / channels, samples.data(), nullptr);    
    if (ma_audio_buffer_init(&bufferConfig, &buffer) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize buffer.");
    }

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize audio device.");
    }

    // Check how long the audio file is and adjust the duration accordingly
    ma_uint64 framesTotal = 0;
    if (ma_audio_buffer_get_length_in_pcm_frames(&buffer, &framesTotal) != MA_SUCCESS) {
        throw std::runtime_error("Failed to get audio file length.");
    }
    if (ma_device_start(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to start audio device.");
    }

    // Wait until the audio has finished playing
    // TODO: add premature stop functionality and save cursor location (maybe use atomic fetch add/subtract)
    while (true) {
        ma_uint64 cursor = 0;
        if (ma_audio_buffer_get_cursor_in_pcm_frames(&buffer, &cursor) != MA_SUCCESS) {
            cursor = MA_UINT64_MAX;
            errorFlag = true;
        }

        if (cursor >= framesTotal) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (errorFlag) {
        throw std::runtime_error("Failed to load cursor.");
    }
    if (ma_device_stop(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to stop audio device.");
    }
    ma_audio_buffer_uninit(&buffer);
    deviceConfig.pUserData = nullptr;
    deviceConfig.dataCallback = nullptr;
    ma_device_uninit(&device);
}

void OutputAudio::setVolumeScale(const float volumeScale) {
    this->volumeScale = volumeScale;
}

ma_format OutputAudio::getFormat() {
    return deviceConfig.playback.format;
}
ma_device_config OutputAudio::getDeviceConfig() {
    return deviceConfig;
}
ma_uint32 OutputAudio::getSampleRate() {
    return sampleRate;
}
ma_uint32 OutputAudio::getChannels() {
    return channels;
}
float OutputAudio::getVolumeScale() {
    return volumeScale;
}

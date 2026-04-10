#include "audio.h"

InputAudio::InputAudio(const ma_uint32 sampleRate, const ma_uint32 channels, const bool isVerbose) :
    sampleRate(sampleRate), channels(channels), isVerbose(isVerbose) {}

void InputAudio::init() {
    if (isVerbose) {
        std::cout << "Starting Input Audio Initilisation" << std::endl;
    }
    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format   = ma_format_f32;
    deviceConfig.capture.channels = channels;
    deviceConfig.sampleRate       = sampleRate;
    audioBuffer.push_back(0);
    deviceConfig.pUserData        = &audioBuffer;

    deviceConfig.dataCallback = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        std::vector<float>* audioData = static_cast<std::vector<float>*>(pDevice->pUserData);

        // Store the raw audio data in a vector
        const float* input = (const float*)pInput;
        for (ma_uint32 i = 0; i < frameCount * 1; i++) {
            audioData->push_back(input[i]);
        }

        (void)pOutput;
    };
}


void InputAudio::recordAudio(const float duration, const bool saveToFile, const char* filename) {
    if (isVerbose) {
        std::cout << "Starting Audio Recording" << std::endl;
    }
     if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize audio device.");
    }
    if (ma_device_start(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to start audio device.");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds((int)(duration * 1000)));
    if (ma_device_stop(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to stop audio device.");
    }

    if (isVerbose) {
        std::cout << "Gain setting at " << gain << "dB" << std::endl;
    }
    // Gain control
    if (gain != 0) {
        float gainFactor = pow(10.0f, gain / 20.0f); // Convert dB to linear
        for (auto& sample : audioBuffer) {
            sample *= gainFactor;
            sample = std::max(-1.0f, std::min(1.0f, sample)); // [-1.0, 1.0]
        }
    }
    
    if (saveToFile) {
        if (isVerbose) {
            std::cout << "Saving to file " << filename << std::endl;
        }
        ma_encoder encoder;
        ma_encoder_config encoderConfig;
        const char* lFilename;

        if (audioBuffer.empty()) {
            audioBuffer.push_back(0.0f); // Ensure one sample to write
        }
        if (filename == nullptr || std::strlen(filename) == 0) {
            lFilename = "recorded_audio.wav"; // Default filename
        } else {
            lFilename = filename;
        }
        encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, channels, sampleRate);

        if (ma_encoder_init_file(lFilename, &encoderConfig, &encoder) != MA_SUCCESS) {
            throw std::runtime_error("Failed to initialize encoder.");
        }
        
        ma_encoder_write_pcm_frames(&encoder, audioBuffer.data(), audioBuffer.size(), NULL);

        ma_encoder_uninit(&encoder);
    }
    ma_device_uninit(&device);
}

// Getters and Setters
void InputAudio::setGain(const int gain) {
    this->gain = gain;
}

std::vector<float> InputAudio::getAudioBuffer() {
    return audioBuffer;
}
ma_device_config InputAudio::getdeviceConfig() {
    return deviceConfig;
}
ma_format InputAudio::getFormat() {
    return deviceConfig.capture.format;
}
ma_uint32 InputAudio::getSampleRate() {
    return sampleRate;
}
ma_uint32 InputAudio::getChannels() {
    return channels;
}
float InputAudio::getGain() {
    return gain;
}

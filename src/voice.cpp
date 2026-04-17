#include "voice.h"

Voice::~Voice() {
    if (synth) {
        piper_free(synth);
        synth = nullptr;
    }
}

Voice::Voice(const std::string& modelPath, const std::string& configPath, const std::string& espeakDataPath,
                uint32_t sampleRate, uint32_t channels, float lengthScale, float noiseScale, float noiseWScale,
                bool isVerbose)
    : modelPath(modelPath), configPath(configPath), espeakDataPath(espeakDataPath),
    sampleRate(sampleRate), channels(channels), isVerbose(isVerbose),
    lengthScale(lengthScale), noiseScale(noiseScale), noiseWScale(noiseWScale) {
}

void Voice::init() {
    if (isVerbose) std::cout << "Initializing voice synthesizer..." << std::endl;
    synth = piper_create(modelPath.c_str(), configPath.c_str(), espeakDataPath.c_str());
    if (synth == nullptr) {
        throw std::runtime_error("Failed to create Piper synthesizer.");
    }
    options = piper_default_synthesize_options(synth);
    options.length_scale = lengthScale; // default 1.0
    options.noise_scale = noiseScale; // default 0.667 for single speaker
    options.noise_w_scale = noiseWScale; // default 0.8 for single speaker
    if (isVerbose) std::cout << "Voice synthesizer initialized." << std::endl;
}

void Voice::synthesise(const std::string text) {
    if (isVerbose) std::cout << "Starting synthesis for text: " << text << std::endl;
    piper_audio_chunk chunk;
    chunk.sample_rate = sampleRate;
    float tempSamples = 0;
    audioData.clear();

    piper_synthesize_start(synth, text.c_str(), &options);
    while (piper_synthesize_next(synth, &chunk) != PIPER_DONE) {
        if (isVerbose) std::cout << "Gain setting at " << gain << "dB" << std::endl;
        // Gain control
        if (gain != 0) {
            float gainFactor = pow(10.0f, gain / 20.0f); // Convert dB to linear
            for (auto& sample : audioData) {
                sample *= gainFactor;
                sample = std::max(-1.0f, std::min(1.0f, sample)); // [-1.0, 1.0]
            }
        }
        if (chunk.num_phoneme_ids > 0) {
            audioData.insert(audioData.end(), chunk.samples, chunk.samples + chunk.num_samples);
        }
    }
}

void Voice::saveToFile(const char* filename) {
    if (isVerbose) std::cout << "Saving to file " << ((filename == nullptr) ? "output_audio.wav" : filename) << std::endl;
    ma_encoder encoder;
    ma_encoder_config encoderConfig;
    const char* lFilename;

    if (audioData.empty()) {
        audioData.push_back(0.0f); // Ensure one sample to write
    }
    if (filename == nullptr || std::strlen(filename) == 0) {
        lFilename = "output_audio.wav"; // Default filename
    } else {
        lFilename = filename;
    }
    encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, (ma_uint32) channels, (ma_uint32) sampleRate);

    if (ma_encoder_init_file(lFilename, &encoderConfig, &encoder) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize encoder.");
    }
    
    ma_encoder_write_pcm_frames(&encoder, audioData.data(), audioData.size(), NULL);

    ma_encoder_uninit(&encoder);
    lFilename = nullptr;
}

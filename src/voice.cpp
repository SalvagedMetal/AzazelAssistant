#include "voice.h"

Voice::~Voice() {
    piper_free(synth);
}

Voice::Voice(const std::string& modelPath, const std::string& configPath, const std::string& espeakDataPath,
                int frequency, std::string fileName, float lengthScale, float noiseScale, float noiseWScale,  bool enabled, bool isVerbose)
    : modelPath(modelPath), configPath(configPath), espeakDataPath(espeakDataPath),
    frequency(frequency), fileName(fileName), enabled(enabled), isVerbose(isVerbose),
    lengthScale(lengthScale), noiseScale(noiseScale), noiseWScale(noiseWScale) {
}

void Voice::init() {
    if (isVerbose) std::cout << "Initializing voice synthesizer..." << std::endl;
    synth = piper_create(modelPath.c_str(), configPath.c_str(), espeakDataPath.c_str());
    options = piper_default_synthesize_options(synth);
    options.length_scale = lengthScale; // default 1.0
    options.noise_scale = noiseScale; // default 0.667 for single speaker
    options.noise_w_scale = noiseWScale; // default 0.8 for single speaker
    if (synth == nullptr) {
        throw std::runtime_error("Failed to create Piper synthesizer.");
    }
    if (isVerbose) std::cout << "Voice synthesizer initialized." << std::endl;
}

void Voice::speak(std::string text) {
    if (isVerbose) std::cout << "Starting synthesis for text: " << text << std::endl;
    piper_synthesize_start(synth, text.c_str(), &options);
    piper_audio_chunk chunk;
    std::vector<float> audio_data;
    float tempSamples = 0;

    audio_stream.open(fileName, std::ios::binary);
    if (!audio_stream.is_open()) {
        throw std::runtime_error("Failed to open audio output file.");
    }
    while (piper_synthesize_next(synth, &chunk) != PIPER_DONE) {
        // Volume scaling
        for (int i = 0; i < chunk.num_samples; i++) {
            tempSamples = chunk.samples[i] * volumeScale;
            if (tempSamples > 1.0f) tempSamples = 1.0f;
            if (tempSamples < -1.0f) tempSamples = -1.0f;
            const_cast<float*>(chunk.samples)[i] = tempSamples;
        }
        audio_stream.write(reinterpret_cast<const char *>(chunk.samples),
                           chunk.num_samples * sizeof(float));
        audio_data.insert(audio_data.end(), chunk.samples, chunk.samples + chunk.num_samples);
    }
    audio_stream.close();
    if (audio_data.empty()) {
        throw std::runtime_error("No audio data was generated.");
    }
    std::string playCommand = "aplay -r " + std::to_string(frequency) + " -c 1 -f FLOAT_LE -t raw " + fileName  + " -s";
    system(playCommand.c_str());
}

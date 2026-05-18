#include "voiceRec.h"

void VoiceRecognition::init() {
    if (isVerbose) std::cout << "Initilising voice recognition" << std::endl;

    //disable log dumping
    whisper_log_set([](ggml_log_level, const char *, void *) {}, nullptr);

    cparams = whisper_context_default_params();
    cparams.use_gpu = use_gpu;

    // Greedy best for low power models
    wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.no_speech_thold = no_speech_thold;
    wparams.max_len = max_len;
    wparams.translate = translate;
    wparams.n_threads = n_threads;
    wparams.no_context = no_context;
    wparams.no_timestamps = no_timestamps;
    wparams.single_segment = single_segment;
    wparams.language = language.c_str();
    wparams.print_progress = print_progress;
    wparams.print_realtime = print_realtime;
    wparams.print_special = print_special;
    wparams.print_timestamps = print_timestamps;

    ctx = whisper_init_from_file_with_params(filePath.c_str(), cparams);
    if (ctx == nullptr) {
        throw std::runtime_error("Whisper Initilisation failed");
    }
    if (isVerbose) std::cout << "Model loaded" << std::endl;
}


void VoiceRecognition::transcribe(std::vector<float> &audioData) {
    std::string tempTranscript = "";
    if (isVerbose) std::cout << "Transcribing audio" << std::endl;
    if (!audioData.empty()) {
        if (whisper_full(ctx, wparams, audioData.data(), audioData.size()) != 0) {
            throw std::runtime_error("Failed to process audio");
        }

        const int n_segments = whisper_full_n_segments(ctx);
        for (int i = 0; i < n_segments; ++i) {
            const char *text = whisper_full_get_segment_text(ctx, i);
            tempTranscript += text;
            if (isVerbose) printf("%s", text);
        }

        if (!tempTranscript.empty() && tempTranscript.front() == ' ') {
            tempTranscript.erase(0, 1);
        }

        transcript = tempTranscript;
        audioData.clear();
    }
}


void VoiceRecognition::clear() {
    whisper_free(ctx);
}

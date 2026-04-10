#include "../src/audio.h"
#include <cassert>

void testInputInit() {
    InputAudio testAudio(48000, 1, true);
    testAudio.init();
    assert(testAudio.getFormat() == ma_format_f32);
}

void testInputRecord() {
    InputAudio testAudio(48000, 1, true);
    testAudio.init();
    testAudio.recordAudio(0, false, nullptr);
}

void testOutputInit() {
    OutputAudio testAudio(48000, 1, true);
    testAudio.init();
    assert(testAudio.getFormat() == ma_format_f32);
}

void testOuputPlayfile() {
    OutputAudio testAudio(48000, 1, true);
    InputAudio testInput(48000, 1, true);
    testInput.init();
    testAudio.init();

    testInput.recordAudio(0, true, "test_file.wav");
    testAudio.playAudioFile("test_file.wav");
}

void testOutputPlayBuffer() {
    OutputAudio testAudio(48000, 1, true);
    InputAudio testInput(48000, 1, true);
    testInput.init();
    testAudio.init();

    testInput.recordAudio(0, false, nullptr);
    testAudio.playAudioBuffer(testInput.getAudioBuffer());
}

int main() {
    std::cout << "Running Audio Tests..." << std::endl;
    try {
        testInputInit();
        testOutputInit();
        testOuputPlayfile();
        testOutputPlayBuffer();
    } catch (const std::exception& e) {
        std::cerr << "Audio Tests Failed: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Audio Tests Passed!" << std::endl;
    return 0;
}

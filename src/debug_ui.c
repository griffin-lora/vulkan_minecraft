#include "debug_ui.h"
#include "vk/text_assets.h"
#include <stdio.h>

#define TEXT_MODEL_INDEX 0
#define NUM_TEXT_MODEL_GLYPHS 128

result_t init_debug_ui(void) {
    if (init_text_model(TEXT_MODEL_INDEX, (vec2s) {{ -0.9f, -0.9f }}, NUM_TEXT_MODEL_GLYPHS) != result_success) {
        return result_failure;
    }

    return result_success;
}

result_t update_debug_ui(microseconds_t frame_time, float delta) {
    char message[NUM_TEXT_MODEL_GLYPHS];
    snprintf(message, NUM_TEXT_MODEL_GLYPHS, "FPS: %f\nFrame Time: %ldms", 1.0f/delta, get_milliseconds(frame_time));

    if (set_text_model_message(TEXT_MODEL_INDEX, message) != result_success) {
        return result_failure;
    }

    return result_success;
}
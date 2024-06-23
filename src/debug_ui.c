#include "debug_ui.h"
#include "vk/text_assets.h"
#include "timing.h"
#include "camera.h"
#include <stdio.h>

#define TEXT_MODEL_INDEX 0
#define NUM_TEXT_MODEL_GLYPHS 256

result_t init_debug_ui(void) {
    result_t result;
    if ((result = init_text_model(TEXT_MODEL_INDEX, (vec2s) {{ -0.9f, -0.9f }}, NUM_TEXT_MODEL_GLYPHS)) != result_success) {
        return result;
    }

    return result_success;
}

result_t update_debug_ui(microseconds_t frame_time, float delta) {
    char message[NUM_TEXT_MODEL_GLYPHS];
    snprintf(message, NUM_TEXT_MODEL_GLYPHS, 
        "FPS: %f\n"
        "POS: %f, %f, %f\n"
        "Frame: %ldms\n"
        "In Flight: %ldms\n"
        "Begin Frame: %ldms\n"
        "Write Command Buffer: %ldms\n"
        "End Frame: %ldms\n",
        1.0f/delta,
        camera_position.x, camera_position.y, camera_position.z,
        get_milliseconds(frame_time),
        get_milliseconds(in_flight_time),
        get_milliseconds(begin_frame_time),
        get_milliseconds(write_command_buffer_time),
        get_milliseconds(end_frame_time)
    );

    if (set_text_model_message(TEXT_MODEL_INDEX, message) != result_success) {
        return result_failure;
    }

    return result_success;
}
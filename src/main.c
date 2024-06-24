#include "result.h"
#include "vk/core.h"
#include "vk/render.h"
#include "camera.h"
#include "debug_ui.h"
#include "chrono.h"
#include "vk/dynamic_assets.h"
#include <stdbool.h>
#include <stdio.h>

int main(void) {
    result_t result;
    if ((result = init_core()) != result_success) {
        print_result_error(result);
        return 1;
    }

    microseconds_t program_start = get_current_microseconds();

    float delta = 1.0f/60.0f;
    while (!glfwWindowShouldClose(window)) {
        microseconds_t start = get_current_microseconds() - program_start;
        glfwPollEvents();

        update_camera(delta);
        
        const char* msg = update_dynamic_assets();
        if (msg != NULL) {
            printf("%s", msg);
            return 1;
        }

        if ((result = draw_frame(delta)) != result_success) {
            print_result_error(result);
            return 1;
        }
        microseconds_t end = get_current_microseconds() - program_start;
        microseconds_t delta_microseconds = end - start;
        delta = (float)delta_microseconds/1000000.0f;

        microseconds_t remaining_microseconds = (1000000l/60l) - delta_microseconds;
        if (remaining_microseconds > 0) {
            sleep_microseconds(remaining_microseconds);
        }

        end = get_current_microseconds() - program_start;
        microseconds_t new_delta_microseconds = end - start;
        delta = (float)new_delta_microseconds/1000000.0f;

        update_debug_ui(delta_microseconds, delta);
    }

    term_all();

    return 0;
}
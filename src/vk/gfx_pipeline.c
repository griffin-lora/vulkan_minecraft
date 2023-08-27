#include "gfx_pipeline.h"
#include "color_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "util.h"
#include "asset.h"

const char* init_vulkan_graphics_pipelines() {
    const char* msg = init_color_pipeline();
    if (msg != NULL) {
        return msg;
    }

    return NULL;
}

void term_vulkan_graphics_pipelines() {
    term_color_pipeline();
}
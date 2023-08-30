#include "gfx_pipeline.h"
#include "voxel_color_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "util.h"
#include "asset.h"

const char* init_vulkan_graphics_pipelines() {
    const char* msg = init_voxel_color_pipeline();
    if (msg != NULL) {
        return msg;
    }

    return NULL;
}

void draw_graphics_pipelines(VkCommandBuffer command_buffer) {
    draw_voxel_color_pipeline(command_buffer);
}

void term_vulkan_graphics_pipelines() {
    term_voxel_color_pipeline();
}
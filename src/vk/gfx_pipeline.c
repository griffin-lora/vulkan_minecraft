#include "gfx_pipeline.h"
#include "voxel_color_pipeline.h"
#include "text_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "util.h"
#include "asset.h"

const char* init_graphics_pipelines() {
    const char* msg = init_voxel_color_pipeline();
    if (msg != NULL) { return msg; }

    msg = init_text_pipeline();
    if (msg != NULL) { return msg; }

    return NULL;
}

void transfer_graphics_pipelines(VkCommandBuffer command_buffer) {
    transfer_text_pipeline(command_buffer);
}

void draw_graphics_pipelines(VkCommandBuffer command_buffer) {
    draw_voxel_color_pipeline(command_buffer);
    draw_text_pipeline(command_buffer);
}

void term_graphics_pipelines() {
    term_voxel_color_pipeline();
    term_text_pipeline();
}
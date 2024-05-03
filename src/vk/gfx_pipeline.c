#include "gfx_pipeline.h"
#include "voxel_color_pipeline.h"
#include "text_pipeline.h"
#include "core.h"

const char* init_graphics_pipelines() {
    const char* msg = init_voxel_color_pipeline();
    if (msg != NULL) { return msg; }

    msg = init_text_pipeline();
    if (msg != NULL) { return msg; }

    return NULL;
}

void draw_graphics_pipelines(VkCommandBuffer command_buffer) {
    draw_voxel_color_pipeline(command_buffer);
    draw_text_pipeline(command_buffer);
}

void term_graphics_pipelines() {
    term_voxel_color_pipeline();
    term_text_pipeline();
}

void destroy_graphics_pipeline(const graphics_pipeline_render_info_t* info) {
    vkDestroyPipeline(device, info->pipeline, NULL);
    vkDestroyPipelineLayout(device, info->pipeline_layout, NULL);
    vkDestroyDescriptorPool(device, info->descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device, info->descriptor_set_layout, NULL);
}
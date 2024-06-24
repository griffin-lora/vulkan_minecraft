#pragma once
#include "result.h"
#include <GLFW/glfw3.h>

typedef struct {
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
} graphics_pipeline_render_info_t;

result_t init_graphics_pipelines(void);
void draw_graphics_pipelines(VkCommandBuffer command_buffer);
void term_graphics_pipelines(void);

void destroy_graphics_pipeline(const graphics_pipeline_render_info_t* info);
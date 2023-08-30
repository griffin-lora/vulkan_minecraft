#pragma once
#include "vk.h"

const char* init_vulkan_graphics_pipelines(void);
void draw_graphics_pipelines(VkCommandBuffer command_buffer);
void term_vulkan_graphics_pipelines(void);
#pragma once
#include "vk.h"

const char* init_graphics_pipelines(void);
void draw_graphics_pipelines(VkCommandBuffer command_buffer);
void term_graphics_pipelines(void);
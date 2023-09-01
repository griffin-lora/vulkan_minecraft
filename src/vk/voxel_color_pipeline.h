#pragma once
#include "vk.h"

const char* init_voxel_color_pipeline(void);
void draw_voxel_color_pipeline(VkCommandBuffer command_buffer);
void term_voxel_color_pipeline(void);
#pragma once
#include "result.h"
#include <GLFW/glfw3.h>

result_t init_voxel_color_pipeline(void);
void draw_voxel_color_pipeline(VkCommandBuffer command_buffer);
void term_voxel_color_pipeline(void);
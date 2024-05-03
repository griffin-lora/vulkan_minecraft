#pragma once
#include "result.h"
#include <GLFW/glfw3.h>

extern VkRenderPass frame_render_pass;
extern VkImageView frame_color_image_view;
extern VkImageView frame_depth_image_view;

result_t init_frame_swapchain_dependents(void);
void term_frame_swapchain_dependents(void);

const char* init_frame_rendering(void);
const char* draw_frame(float delta);
void term_frame_rendering(void);
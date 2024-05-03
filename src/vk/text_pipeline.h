#pragma once
#include <GLFW/glfw3.h>

const char* init_text_pipeline(void);
void draw_text_pipeline(VkCommandBuffer command_buffer);
void term_text_pipeline(void);
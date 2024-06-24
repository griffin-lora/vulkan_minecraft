#pragma once
#include "result.h"
#include <GLFW/glfw3.h>

result_t init_text_pipeline(void);
void draw_text_pipeline(VkCommandBuffer command_buffer);
void term_text_pipeline(void);
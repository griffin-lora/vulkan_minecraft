#pragma once
#include "vk.h"

const char* init_text_pipeline(void);
void draw_text_pipeline(VkCommandBuffer command_buffer);
void term_text_pipeline(void);
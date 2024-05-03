#pragma once
#include "result.h"
#include <GLFW/glfw3.h>
#include <pthread.h>

extern VkCommandPool dynamic_asset_transfer_command_pool;
extern VkCommandBuffer dynamic_asset_transfer_command_buffer;

const char* init_dynamic_asset_transfer(void);

result_t begin_dynamic_asset_transfer(void);
result_t end_dynamic_asset_transfer(void);

void term_dynamic_asset_transfer(void);
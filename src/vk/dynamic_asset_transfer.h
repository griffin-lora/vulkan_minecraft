#pragma once
#include "vk.h"
#include "result.h"
#include <pthread.h>

extern VkCommandBuffer dynamic_asset_transfer_command_buffer;

const char* init_dynamic_asset_transfer(void);

result_t begin_dynamic_asset_transfer(void);
result_t end_dynamic_asset_transfer(void);

void term_dynamic_asset_transfer(void);
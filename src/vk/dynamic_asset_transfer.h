#pragma once
#include "vk.h"
#include "result.h"
#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    size_t index;
} dynamic_assets_front_index_t;

#define DYNAMIC_ASSETS_FRONT_INDEX_INITIALIZER {\
    .mutex = PTHREAD_MUTEX_INITIALIZER,\
    .index = 0\
}

extern VkCommandBuffer dynamic_asset_transfer_command_buffer;

const char* init_dynamic_asset_transfer(void);

result_t begin_dynamic_asset_transfer(void);
result_t end_dynamic_asset_transfer(void);

void term_dynamic_asset_transfer(void);
#pragma once
#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    size_t index;
} dynamic_assets_front_index_t;

#define DYNAMIC_ASSETS_FRONT_INDEX_INITIALIZER {\
    .mutex = PTHREAD_MUTEX_INITIALIZER,\
    .index = 0\
}

const char* update_dynamic_assets(void);
void term_dynamic_assets(void);
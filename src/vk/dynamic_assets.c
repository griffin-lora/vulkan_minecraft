#include "dynamic_assets.h"
#include "dynamic_asset_transfer.h"
#include "result.h"
#include "voxel_dynamic_assets.h"
#include "core.h"
#include <pthread.h>
#include <stdatomic.h>

bool should_recreate_voxel_regions = true;

static struct {
    bool should_recreate_voxel_regions;
} update_dynamic_assets_thread_info;

static bool has_update_dynamic_assets_thread = false;
static atomic_bool update_dynamic_assets_thread_active = false;
static pthread_t update_dynamic_assets_thread;

static result_t update_dynamic_assets_thread_core() {
    result_t result;

    if (update_dynamic_assets_thread_info.should_recreate_voxel_regions && (result = begin_voxel_regions()) != result_success) {
        update_dynamic_assets_thread_active = false;
        return result;
    }

    if ((result = begin_dynamic_asset_transfer()) != result_success) {
        update_dynamic_assets_thread_active = false;
        return result;
    }

    if (update_dynamic_assets_thread_info.should_recreate_voxel_regions) {
        transfer_voxel_regions();
    }

    if ((result = end_dynamic_asset_transfer()) != result_success) {
        update_dynamic_assets_thread_active = false;
        return result;
    }

    if (update_dynamic_assets_thread_info.should_recreate_voxel_regions) {
        end_voxel_regions();
    }

    update_dynamic_assets_thread_active = false;
    return result_success;
}

static void* update_dynamic_assets_thread_main(void*) {
    result_t result = update_dynamic_assets_thread_core();
    if (result != result_success) {
        print_result_error(result);
    }

    return NULL;
}

const char* update_dynamic_assets(void) {
    if (!should_recreate_voxel_regions) {
        return NULL;
    }

    if (update_dynamic_assets_thread_active) {
        return NULL;
    }

    update_dynamic_assets_thread_active = true;
    
    if (has_update_dynamic_assets_thread) {
        union {
            const char* msg;
            void* data;
        } ret;

        pthread_join(update_dynamic_assets_thread, &ret.data);
        if (ret.msg != NULL) {
            return ret.msg;
        }
    }
    has_update_dynamic_assets_thread = true;

    update_dynamic_assets_thread_info.should_recreate_voxel_regions = should_recreate_voxel_regions;
    pthread_create(&update_dynamic_assets_thread, NULL, update_dynamic_assets_thread_main, NULL);

    should_recreate_voxel_regions = false;

    return NULL;
}

void term_update_dynamic_assets_thread(void) {
    pthread_mutex_lock(&command_buffer_finished_mutex);
    pthread_cond_signal(&command_buffer_finished_condition);
    pthread_mutex_unlock(&command_buffer_finished_mutex);

    void* data;
    pthread_join(update_dynamic_assets_thread, &data);
}

void term_dynamic_assets(void) {
    term_voxel_dynamic_assets();
}
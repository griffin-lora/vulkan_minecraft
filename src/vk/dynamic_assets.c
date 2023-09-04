#include "dynamic_assets.h"
#include "dynamic_asset_transfer.h"
#include "voxel_dynamic_assets.h"
#include "camera.h"
#include <pthread.h>

bool should_recreate_voxel_regions = true;

typedef struct {
    bool should_recreate_voxel_regions;
} update_dynamic_assets_thread_info_t;

static bool has_update_dynamic_assets_thread = false;
static pthread_t update_dynamic_assets_thread;

void* update_dynamic_assets_thread_main(void* p) {
    const update_dynamic_assets_thread_info_t* info = p;

    if (info->should_recreate_voxel_regions && begin_voxel_regions() != result_success) {
        return "Failed to begin recreating voxel regions\n";
    }

    if (begin_dynamic_asset_transfer() != result_success) {
        return "Failed to begin dynamic asset transfer\n";
    }

    if (info->should_recreate_voxel_regions) {
        transfer_voxel_regions();
    }

    if (end_dynamic_asset_transfer() != result_success) {
        return "Failed to end dynamic asset transfer\n";
    }

    if (info->should_recreate_voxel_regions) {
        end_voxel_regions();
    }

    return NULL;
}

const char* update_dynamic_assets(void) {
    if (!should_recreate_voxel_regions) {
        return NULL;
    }

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

    update_dynamic_assets_thread_info_t info;
    pthread_create(&update_dynamic_assets_thread, NULL, update_dynamic_assets_thread_main, &info);

    should_recreate_voxel_regions = false;

    return NULL;
}

void term_dynamic_assets(void) {
    void* data;
    pthread_join(update_dynamic_assets_thread, &data);

    term_voxel_dynamic_assets();
}
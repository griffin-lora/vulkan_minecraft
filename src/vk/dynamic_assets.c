#include "dynamic_assets.h"
#include "dynamic_asset_transfer.h"
#include "voxel_dynamic_assets.h"
#include "camera.h"

const char* update_dynamic_assets(void) {
    if (!should_recreate_voxel_regions) {
        return NULL;
    }

    if (should_recreate_voxel_regions && begin_voxel_regions() != result_success) {
        return "Failed to begin recreating voxel regions\n";
    }

    if (begin_dynamic_asset_transfer() != result_success) {
        return "Failed to begin dynamic asset transfer\n";
    }

    if (should_recreate_voxel_regions) {
        transfer_voxel_regions();
    }

    if (end_dynamic_asset_transfer() != result_success) {
        return "Failed to end dynamic asset transfer\n";
    }

    if (should_recreate_voxel_regions) {
        end_voxel_regions();
    }

    should_recreate_voxel_regions = false;

    return NULL;
}

void term_dynamic_assets(void) {
    term_voxel_dynamic_assets();
}
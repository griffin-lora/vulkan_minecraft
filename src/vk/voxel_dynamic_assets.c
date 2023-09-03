#include "voxel_dynamic_assets.h"
#include "voxel_dynamic_asset_transfer.h"
#include "voxel/region.h"
#include <stdalign.h>
#include <malloc.h>
#include <string.h>

dynamic_assets_front_index_t voxel_dynamic_assets_front_index = DYNAMIC_ASSETS_FRONT_INDEX_INITIALIZER;
alignas(64) voxel_region_render_info_t voxel_region_render_info_arrays[NUM_QUEUED_VOXEL_DYNAMIC_ASSSETS][NUM_VOXEL_REGIONS] = { 0 };
alignas(64) voxel_region_allocation_info_t voxel_region_allocation_info_arrays[NUM_QUEUED_VOXEL_DYNAMIC_ASSSETS][NUM_VOXEL_REGIONS] = { 0 };

static staging_t region_stagings[NUM_VOXEL_REGIONS];
static size_t num_inits = 0;

result_t begin_voxel_regions(void) {
    size_t back_index = voxel_dynamic_assets_front_index.index ^ 1;

    voxel_region_render_info_t* render_infos = voxel_region_render_info_arrays[back_index];
    voxel_region_allocation_info_t* allocation_infos = voxel_region_allocation_info_arrays[back_index];

    voxel_region_voxel_type_array_t (*voxel_type_arrays)[NUM_XZ_VOXEL_REGIONS] = memalign(64, NUM_VOXEL_REGIONS*sizeof(voxel_region_voxel_type_array_t));
    memset(voxel_type_arrays, voxel_type_air, NUM_VOXEL_REGIONS*sizeof(voxel_region_voxel_type_array_t));

    {
        size_t i = 0;
        for (size_t x = 0; x < NUM_XZ_VOXEL_REGIONS; x++)
        for (size_t z = 0; z < NUM_XZ_VOXEL_REGIONS; z++, i++) {
            voxel_region_render_info_t* render_info = &render_infos[i];

            render_info->position = (vec3s) {{ (float)(x * VOXEL_REGION_SIZE), 0.0f, (float)(z * VOXEL_REGION_SIZE) }};

            voxel_region_voxel_type_array_t* voxel_types = &voxel_type_arrays[x][z];

            create_voxel_region_voxel_type_array(num_inits*1000, x*VOXEL_REGION_SIZE, 0, z*VOXEL_REGION_SIZE, voxel_types);
        }

        i = 0;
        for (size_t x = 0; x < NUM_XZ_VOXEL_REGIONS; x++)
        for (size_t z = 0; z < NUM_XZ_VOXEL_REGIONS; z++, i++) {
            voxel_vertex_array_t vertex_array = { 0 };
            create_voxel_vertex_array(&(voxel_region_voxel_type_arrays_t) {
                .center = &voxel_type_arrays[x][z],
                .front = x + 1u < NUM_XZ_VOXEL_REGIONS ? &voxel_type_arrays[x + 1][z] : NULL,
                .back = x - 1u < NUM_XZ_VOXEL_REGIONS ? &voxel_type_arrays[x - 1][z] : NULL,
                .right = z + 1u < NUM_XZ_VOXEL_REGIONS ? &voxel_type_arrays[x][z + 1] : NULL,
                .left = z - 1u < NUM_XZ_VOXEL_REGIONS ? &voxel_type_arrays[x][z - 1] : NULL,
            }, &vertex_array);

            if (begin_voxel_region_info(&vertex_array, &region_stagings[i], &render_infos[i], &allocation_infos[i]) != result_success) {
                return result_failure;
            }
        }
    }

    free(voxel_type_arrays);

    num_inits++;

    return result_success;
}

void transfer_voxel_regions(void) {
    size_t back_index = voxel_dynamic_assets_front_index.index ^ 1;

    const voxel_region_render_info_t* render_infos = voxel_region_render_info_arrays[back_index];

    for (size_t i = 0; i < NUM_VOXEL_REGIONS; i++) {
        transfer_voxel_region_info(&region_stagings[i], &render_infos[i]);
    }
}

void end_voxel_regions(void) {
    for (size_t i = 0; i < NUM_VOXEL_REGIONS; i++) {
        end_voxel_region_info(&region_stagings[i]);
    }

    pthread_mutex_lock(&voxel_dynamic_assets_front_index.mutex);
    voxel_dynamic_assets_front_index.index ^= 1; // swap back and front
    pthread_mutex_unlock(&voxel_dynamic_assets_front_index.mutex);

    size_t back_index = voxel_dynamic_assets_front_index.index ^ 1;

    voxel_region_render_info_t* render_infos = voxel_region_render_info_arrays[back_index];
    voxel_region_allocation_info_t* allocation_infos = voxel_region_allocation_info_arrays[back_index];

    for (size_t i = 0; i < NUM_VOXEL_REGIONS; i++) {
        destroy_voxel_region_info(&render_infos[i], &allocation_infos[i]);
    }
}

void term_voxel_dynamic_assets(void) {
    for (size_t i = 0; i < NUM_QUEUED_VOXEL_DYNAMIC_ASSSETS; i++) {
        voxel_region_render_info_t* render_infos = voxel_region_render_info_arrays[i];
        voxel_region_allocation_info_t* allocation_infos = voxel_region_allocation_info_arrays[i];

        for (size_t j = 0; j < NUM_VOXEL_REGIONS; j++) {
            destroy_voxel_region_info(&render_infos[i], &allocation_infos[i]);
        }
    }
}
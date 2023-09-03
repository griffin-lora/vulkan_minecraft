#include "voxel_assets.h"
#include "voxel/vertex.h"
#include "voxel/region.h"
#include "core.h"
#include "defaults.h"
#include <stdalign.h>
#include <malloc.h>
#include <string.h>

alignas(64) voxel_region_render_info_t voxel_region_render_infos[NUM_VOXEL_REGIONS] = { 0 };
alignas(64) voxel_region_allocation_info_t voxel_region_allocation_infos[NUM_VOXEL_REGIONS] = { 0 };

VkSampler voxel_region_texture_image_sampler;

typedef struct {
    staging_t region_vertex_stagings[NUM_VOXEL_REGIONS];
} voxel_assets_info_t;

static voxel_assets_info_t* info;

const char* begin_voxel_assets(float max_anistropy, uint32_t num_mip_levels) {
    info = memalign(64, sizeof(voxel_assets_info_t));
    memset(info, 0, sizeof(voxel_assets_info_t));

    if (vkCreateSampler(device, &(VkSamplerCreateInfo) {
        DEFAULT_VK_SAMPLER,
        .maxAnisotropy = max_anistropy,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .minFilter = VK_FILTER_NEAREST,
        .magFilter = VK_FILTER_NEAREST,
        .anisotropyEnable = VK_FALSE,
        .maxLod = (float)num_mip_levels
    }, NULL, &voxel_region_texture_image_sampler) != VK_SUCCESS) {
        return "Failed to create tetxure image sampler\n";
    }

    voxel_region_voxel_type_array_t (*voxel_type_arrays)[NUM_XZ_VOXEL_REGIONS] = memalign(64, NUM_VOXEL_REGIONS*sizeof(voxel_region_voxel_type_array_t));
    memset(voxel_type_arrays, voxel_type_air, NUM_VOXEL_REGIONS*sizeof(voxel_region_voxel_type_array_t));

    {
        size_t i = 0;
        for (size_t x = 0; x < NUM_XZ_VOXEL_REGIONS; x++)
        for (size_t z = 0; z < NUM_XZ_VOXEL_REGIONS; z++, i++) {
            voxel_region_render_info_t* render_info = &voxel_region_render_infos[i];

            render_info->position = (vec3s) {{ (float)(x * VOXEL_REGION_SIZE), 0.0f, (float)(z * VOXEL_REGION_SIZE) }};

            voxel_region_voxel_type_array_t* voxel_types = &voxel_type_arrays[x][z];

            create_voxel_region_voxel_type_array(x*VOXEL_REGION_SIZE, 0, z*VOXEL_REGION_SIZE, voxel_types);
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

            if (begin_voxel_region_info(&vertex_array, &info->region_vertex_stagings[i], &voxel_region_render_infos[i], &voxel_region_allocation_infos[i]) != result_success) {
                return "Failed to begin creating voxel region info\n";
            }
        }
    }

    free(voxel_type_arrays);

    return NULL;
}

void transfer_voxel_assets(VkCommandBuffer command_buffer) {
    for (size_t i = 0; i < NUM_VOXEL_REGIONS; i++) {
        const voxel_region_render_info_t* render_info = &voxel_region_render_infos[i];

        if (render_info->num_vertices != 0) {
            vkCmdCopyBuffer(command_buffer, info->region_vertex_stagings[i].buffer, render_info->vertex_buffer, 1, &(VkBufferCopy) {
                .size = sizeof(voxel_vertex_t)*render_info->num_vertices
            });
        }
    }
}

void end_voxel_assets() {
    for (size_t i = 0; i < NUM_VOXEL_REGIONS; i++) {
        const staging_t* staging = &info->region_vertex_stagings[i];
        vmaDestroyBuffer(allocator, staging->buffer, staging->allocation);
    }

    free(info);
}

void term_voxel_assets(void) {
    vkDestroySampler(device, voxel_region_texture_image_sampler, NULL);

    for (size_t i = 0; i < NUM_VOXEL_REGIONS; i++) {
        const voxel_region_render_info_t* render_info = &voxel_region_render_infos[i];
        const voxel_region_allocation_info_t* allocation_info = &voxel_region_allocation_infos[i];

        if (render_info->num_vertices != 0) {
            vmaDestroyBuffer(allocator, render_info->vertex_buffer, allocation_info->vertex_allocation);
        }
    }
}
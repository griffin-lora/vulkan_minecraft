#include "region.h"
#include "util.h"
#include "vk/core.h"
#include "vk/defaults.h"
#include <malloc.h>
#include <string.h>
#include <vk_mem_alloc.h>

void create_voxel_face_instance_arrays(const voxel_region_voxel_types_t* voxel_types, voxel_face_instance_arrays_t* instance_arrays) {
    // TODO: Actually create face instance arrays correctly
    (void)voxel_types;

    voxel_face_instance_array_t* instance_array = &instance_arrays->arrays[2];
    instance_array->chunk = memalign(64, sizeof(voxel_face_instance_chunk_t));
    instance_array->num_instances = 64;

    voxel_face_instance_t* face_instances = instance_array->chunk->face_instances;
    {
        size_t i = 0;
        for (uint8_t x = 0; x < 8; x++) {
            for (uint8_t y = 0; y < 8; y++, i++) {
                face_instances[i] = (voxel_face_instance_t) {
                    .position = { x, 0, y },
                    .texture_array_index = i % 2
                };
            }
        }
    }
}

result_t begin_voxel_region_info(voxel_face_instance_arrays_t* instance_arrays, voxel_region_staging_t* staging, voxel_region_render_info_t* render_info, voxel_region_allocation_info_t* allocation_info) {
    for (size_t i = 0; i < NUM_VOXEL_FACE_TYPES; i++) {
        voxel_face_instance_array_t* instance_array = &instance_arrays->arrays[i];
        staging_t* face_model_staging = &staging->face_model_stagings[i];
        voxel_face_model_render_info_t* model_render_info = &render_info->face_model_infos[i];
        voxel_face_model_allocation_info_t* model_allocation_info = &allocation_info->face_model_infos[i];
        
        uint32_t num_instances = instance_array->num_instances;
        if (num_instances == 0) {
            continue;
        }

        model_render_info->num_instances = num_instances;

        uint32_t num_array_bytes = num_instances*sizeof(voxel_face_instance_t);

        if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
            DEFAULT_VK_STAGING_BUFFER,
            .size = num_array_bytes
        }, &staging_allocation_create_info, &face_model_staging->buffer, &face_model_staging->allocation, NULL) != VK_SUCCESS) {
            return result_failure;
        }

        if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
            DEFAULT_VK_VERTEX_BUFFER,
            .size = num_array_bytes
        }, &device_allocation_create_info, &model_render_info->instance_buffer, &model_allocation_info->instance_allocation, NULL) != VK_SUCCESS) {
            return result_failure;
        }

        uint32_t num_chunks = div_ceil_uint32(num_instances, NUM_VOXEL_FACE_INSTANCE_CHUNK_MEMBERS); // Integer ceiling division
        uint32_t num_last_chunk_instances = num_instances % NUM_VOXEL_FACE_INSTANCE_CHUNK_MEMBERS;

        void* mapped_data;
        if (vmaMapMemory(allocator, face_model_staging->allocation, &mapped_data) != VK_SUCCESS) {
            return result_failure;
        }

        voxel_face_instance_chunk_t* chunk = instance_array->chunk;
        for (uint32_t j = 0; j < num_chunks; j++) {
            uint32_t num_chunk_instances = j == num_chunks - 1u ? num_last_chunk_instances : NUM_VOXEL_FACE_INSTANCE_CHUNK_MEMBERS;
            memcpy(mapped_data + j*sizeof(chunk->face_instances), chunk->face_instances, num_chunk_instances*sizeof(voxel_face_instance_t));

            voxel_face_instance_chunk_t* new_chunk = chunk->next;
            free(chunk);
            chunk = new_chunk;
        }

        vmaUnmapMemory(allocator, face_model_staging->allocation);
    }

    return result_success;
}
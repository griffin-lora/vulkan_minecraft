#include "region.h"
#include "util.h"
#include "cube.h"
#include "vk/core.h"
#include "vk/defaults.h"
#include <malloc.h>
#include <string.h>
#include <vk_mem_alloc.h>

typedef struct {
    uint32_t num_instances;
    voxel_face_instance_chunk_t* chunk;
} current_voxel_face_instance_chunk_info_t;

static void add_face_instance(voxel_face_instance_t instance, voxel_face_instance_array_t* instance_array, current_voxel_face_instance_chunk_info_t* current_chunk_info) {
    if (instance_array->num_instances == 0) {
        current_chunk_info->chunk = memalign(64, sizeof(voxel_face_instance_chunk_t));
        instance_array->chunk = current_chunk_info->chunk;
    }

    if (current_chunk_info->num_instances >= NUM_VOXEL_FACE_INSTANCE_CHUNK_MEMBERS) {
        voxel_face_instance_chunk_t* new_chunk = memalign(64, sizeof(voxel_face_instance_chunk_t));
        current_chunk_info->chunk->next = new_chunk;
        current_chunk_info->chunk = new_chunk;

        current_chunk_info->num_instances = 0;
    }

    instance_array->num_instances++;
    current_chunk_info->chunk->face_instances[current_chunk_info->num_instances++] = instance;
}

void create_voxel_face_instance_arrays(const voxel_region_voxel_types_t* voxel_types, voxel_face_instance_arrays_t* instance_arrays) {
    current_voxel_face_instance_chunk_info_t current_chunk_infos[NUM_VOXEL_FACE_TYPES] = { 0 };

    for (uint8_t x = 0; x < VOXEL_REGION_SIZE; x++)
    for (uint8_t y = 0; y < VOXEL_REGION_SIZE; y++)
    for (uint8_t z = 0; z < VOXEL_REGION_SIZE; z++) {
        voxel_type_t type = voxel_types->types[x][y][z];
        if (type == voxel_type_air) {
            continue;
        }

        for (size_t i = CUBE_VOXEL_TYPE_BEGIN; i < CUBE_VOXEL_TYPE_END; i++) {
            voxel_face_instance_array_t* instance_array = &instance_arrays->arrays[i];
            current_voxel_face_instance_chunk_info_t* current_chunk_info = &current_chunk_infos[i];

            add_face_instance((voxel_face_instance_t) {
                .position = { x, y, z },
                .texture_array_index = 0
            }, instance_array, current_chunk_info);
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
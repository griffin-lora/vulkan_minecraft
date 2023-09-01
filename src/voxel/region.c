#include "region.h"
#include "util.h"
#include "cube.h"
#include "vk/core.h"
#include "vk/defaults.h"
#include <malloc.h>
#include <string.h>
#include <vk_mem_alloc.h>
#include <stb_perlin.h>

void create_voxel_region_voxel_type_array(size_t region_x, size_t region_y, size_t region_z, voxel_region_voxel_type_array_t* voxel_types) {
    float amplitude = 20.0f;
    float frequency = 0.01f;
    float lacunarity = 3.0f;
    float gain = 0.4f;
    int octaves = 6;

    for (uint8_t x = 0; x < VOXEL_REGION_SIZE; x++)
    for (uint8_t z = 0; z < VOXEL_REGION_SIZE; z++) {
        float height = amplitude * ((stb_perlin_fbm_noise3((float)(x + region_x)*frequency, (float)region_y, (float)(z + region_z)*frequency, lacunarity, gain, octaves) + 1.0f) / 2.0f);
        uint32_t world_height = (uint32_t)height;
        for (uint32_t y = 0; y < VOXEL_REGION_SIZE; y++) {
            voxel_type_t* type = &voxel_types->types[x][y][z];

            if (x == 15 && z == 15) {
                *type = voxel_type_grass;
                continue;
            }

            if (y == world_height) {
                if (x == 15 && z >= 15) {
                    *type = voxel_type_stone; // z axis
                    continue;
                }
                if (z == 15 && x >= 15) {
                    *type = voxel_type_dirt; // x axis
                    continue;
                }
                *type = voxel_type_grass;
                continue;
            }
            if (y < world_height) {
                *type = voxel_type_dirt;
            }
        }
    }
}

static uint8_t get_texture_array_index(voxel_type_t voxel_type, size_t face_type) {
    switch (voxel_type) {
        case voxel_type_grass: switch (face_type) {
            default: return 4;
            case CUBE_VOXEL_FACE_TYPE_TOP: return 0;
            case CUBE_VOXEL_FACE_TYPE_BOTTOM: return 2;
        }
        case voxel_type_stone: return 1;
        case voxel_type_dirt: return 2;
    }
    return 0;
}

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

void create_voxel_face_instance_arrays(const voxel_region_voxel_type_arrays_t* voxel_type_arrays, voxel_face_instance_arrays_t* instance_arrays) {
    const voxel_region_voxel_type_array_t* voxel_types = voxel_type_arrays->center;

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

            switch (i) {
                case CUBE_VOXEL_FACE_TYPE_FRONT:
                    if (x + 1u < VOXEL_REGION_SIZE && voxel_types->types[x + 1][y][z] != voxel_type_air) {
                        continue;
                    }
                    if (x + 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->front->types != NULL && voxel_type_arrays->front->types[0][y][z] != voxel_type_air) {
                        continue;
                    }
                    break;
                case CUBE_VOXEL_FACE_TYPE_BACK:
                    if (x - 1u < VOXEL_REGION_SIZE && voxel_types->types[x - 1][y][z] != voxel_type_air) {
                        continue;
                    }
                    if (x - 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->back->types != NULL && voxel_type_arrays->back->types[VOXEL_REGION_SIZE - 1][y][z] != voxel_type_air) {
                        continue;
                    }
                    break;
                case CUBE_VOXEL_FACE_TYPE_TOP:
                    if (y + 1u < VOXEL_REGION_SIZE && voxel_types->types[x][y + 1][z] != voxel_type_air) {
                        continue;
                    }
                    if (y + 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->top->types != NULL && voxel_type_arrays->top->types[x][0][z] != voxel_type_air) {
                        continue;
                    }
                    break;
                case CUBE_VOXEL_FACE_TYPE_BOTTOM:
                    if (y - 1u < VOXEL_REGION_SIZE && voxel_types->types[x][y - 1][z] != voxel_type_air) {
                        continue;
                    }
                    if (y - 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->bottom->types != NULL && voxel_type_arrays->bottom->types[x][VOXEL_REGION_SIZE - 1][z] != voxel_type_air) {
                        continue;
                    }
                    break;
                case CUBE_VOXEL_FACE_TYPE_RIGHT:
                    if (z + 1u < VOXEL_REGION_SIZE && voxel_types->types[x][y][z + 1] != voxel_type_air) {
                        continue;
                    }
                    if (z + 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->right->types != NULL && voxel_type_arrays->right->types[x][y][0] != voxel_type_air) {
                        continue;
                    }
                    break;
                case CUBE_VOXEL_FACE_TYPE_LEFT:
                    if (z - 1u < VOXEL_REGION_SIZE && voxel_types->types[x][y][z - 1] != voxel_type_air) {
                        continue;
                    }
                    if (z - 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->left->types != NULL && voxel_type_arrays->left->types[x][y][VOXEL_REGION_SIZE - 1] != voxel_type_air) {
                        continue;
                    }
                    break;
            }

            add_face_instance((voxel_face_instance_t) {
                .position = { x, y, z },
                .texture_array_index = get_texture_array_index(type, i)
            }, instance_array, current_chunk_info);
        }
    }
}

result_t begin_voxel_face_model_info(voxel_face_instance_array_t* instance_array, staging_t* staging, voxel_face_model_render_info_t* render_info, voxel_face_model_allocation_info_t* allocation_info) {
    uint32_t num_instances = instance_array->num_instances;
    if (num_instances == 0) {
        return result_success;
    }

    render_info->num_instances = num_instances;

    uint32_t num_array_bytes = num_instances*sizeof(voxel_face_instance_t);

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_STAGING_BUFFER,
        .size = num_array_bytes
    }, &staging_allocation_create_info, &staging->buffer, &staging->allocation, NULL) != VK_SUCCESS) {
        return result_failure;
    }

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_VERTEX_BUFFER,
        .size = num_array_bytes
    }, &device_allocation_create_info, &render_info->instance_buffer, &allocation_info->instance_allocation, NULL) != VK_SUCCESS) {
        return result_failure;
    }

    uint32_t num_chunks = div_ceil_uint32(num_instances, NUM_VOXEL_FACE_INSTANCE_CHUNK_MEMBERS); // Integer ceiling division
    uint32_t num_last_chunk_instances = num_instances % NUM_VOXEL_FACE_INSTANCE_CHUNK_MEMBERS;
    if (num_last_chunk_instances == 0) {
        num_last_chunk_instances = NUM_VOXEL_FACE_INSTANCE_CHUNK_MEMBERS;
    }

    void* mapped_data;
    if (vmaMapMemory(allocator, staging->allocation, &mapped_data) != VK_SUCCESS) {
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

    vmaUnmapMemory(allocator, staging->allocation);

    return result_success;
}
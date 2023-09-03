#include "region.h"
#include "util.h"
#include "face.h"
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

    for (size_t x = 0; x < VOXEL_REGION_SIZE; x++)
    for (size_t z = 0; z < VOXEL_REGION_SIZE; z++) {
        float height = amplitude * ((stb_perlin_fbm_noise3((float)(x + region_x)*frequency, (float)region_y, (float)(z + region_z)*frequency, lacunarity, gain, octaves) + 1.0f) / 2.0f);
        size_t world_height = (size_t)height;
        for (size_t y = 0; y < VOXEL_REGION_SIZE; y++) {
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

static uint8_t get_texture_array_index(voxel_type_t voxel_type, voxel_face_t face) {
    switch (voxel_type) {
        case voxel_type_grass: switch (face) {
            default: return 4;
            case voxel_face_top: return 0;
            case voxel_face_bottom: return 2;
        }
        case voxel_type_stone: return 1;
        case voxel_type_dirt: return 2;
    }
    return 0;
}

typedef struct {
    uint32_t num_vertices;
    voxel_vertex_chunk_t* chunk;
} current_voxel_vertex_chunk_info_t;

static void add_vertices(size_t num_vertices, voxel_vertex_t vertex, voxel_vertex_array_t* vertex_array, current_voxel_vertex_chunk_info_t* current_chunk_info) {
    // TODO: Don't do this
    for (size_t i = 0; i < num_vertices; i++) {
        if (vertex_array->num_vertices == 0) {
            current_chunk_info->chunk = memalign(64, sizeof(voxel_vertex_chunk_t));
            vertex_array->chunk = current_chunk_info->chunk;
        }

        if (current_chunk_info->num_vertices >= NUM_VOXEL_VERTEX_CHUNK_MEMBERS) {
            voxel_vertex_chunk_t* new_chunk = memalign(64, sizeof(voxel_vertex_chunk_t));
            current_chunk_info->chunk->next = new_chunk;
            current_chunk_info->chunk = new_chunk;

            current_chunk_info->num_vertices = 0;
        }

        vertex_array->num_vertices++;
        current_chunk_info->chunk->vertices[current_chunk_info->num_vertices++] = vertex;
    }
}

void create_voxel_vertex_array(const voxel_region_voxel_type_arrays_t* voxel_type_arrays, voxel_vertex_array_t* vertex_array) {
    const voxel_region_voxel_type_array_t* voxel_types = voxel_type_arrays->center;

    current_voxel_vertex_chunk_info_t current_chunk_info = { 0 };

    for (size_t x = 0; x < VOXEL_REGION_SIZE; x++)
    for (size_t y = 0; y < VOXEL_REGION_SIZE; y++)
    for (size_t z = 0; z < VOXEL_REGION_SIZE; z++) {
        voxel_type_t type = voxel_types->types[x][y][z];
        if (type == voxel_type_air) {
            continue;
        }

        for (voxel_type_t i = CUBE_VOXEL_TYPE_BEGIN; i < CUBE_VOXEL_TYPE_END; i++) {
            switch (i) {
                case voxel_face_front:
                    if (x + 1u < VOXEL_REGION_SIZE && voxel_types->types[x + 1][y][z] != voxel_type_air) {
                        continue;
                    }
                    if (x + 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->front->types != NULL && voxel_type_arrays->front->types[0][y][z] != voxel_type_air) {
                        continue;
                    }
                    break;
                case voxel_face_back:
                    if (x - 1u < VOXEL_REGION_SIZE && voxel_types->types[x - 1][y][z] != voxel_type_air) {
                        continue;
                    }
                    if (x - 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->back->types != NULL && voxel_type_arrays->back->types[VOXEL_REGION_SIZE - 1][y][z] != voxel_type_air) {
                        continue;
                    }
                    break;
                case voxel_face_top:
                    if (y + 1u < VOXEL_REGION_SIZE && voxel_types->types[x][y + 1][z] != voxel_type_air) {
                        continue;
                    }
                    if (y + 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->top->types != NULL && voxel_type_arrays->top->types[x][0][z] != voxel_type_air) {
                        continue;
                    }
                    break;
                case voxel_face_bottom:
                    if (y - 1u < VOXEL_REGION_SIZE && voxel_types->types[x][y - 1][z] != voxel_type_air) {
                        continue;
                    }
                    if (y - 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->bottom->types != NULL && voxel_type_arrays->bottom->types[x][VOXEL_REGION_SIZE - 1][z] != voxel_type_air) {
                        continue;
                    }
                    break;
                case voxel_face_right:
                    if (z + 1u < VOXEL_REGION_SIZE && voxel_types->types[x][y][z + 1] != voxel_type_air) {
                        continue;
                    }
                    if (z + 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->right->types != NULL && voxel_type_arrays->right->types[x][y][0] != voxel_type_air) {
                        continue;
                    }
                    break;
                case voxel_face_left:
                    if (z - 1u < VOXEL_REGION_SIZE && voxel_types->types[x][y][z - 1] != voxel_type_air) {
                        continue;
                    }
                    if (z - 1u >= VOXEL_REGION_SIZE && voxel_type_arrays->left->types != NULL && voxel_type_arrays->left->types[x][y][VOXEL_REGION_SIZE - 1] != voxel_type_air) {
                        continue;
                    }
                    break;
            }
            
            add_vertices(NUM_CUBE_VOXEL_FACE_VERTICES, create_voxel_vertex((voxel_vertex_create_info_t) {
                .texture_array_index = get_texture_array_index(type, i),
                .face = i,
                .position = { (uint8_t)x, (uint8_t)y, (uint8_t)z }
            }), vertex_array, &current_chunk_info);
        }
    }
}

result_t begin_voxel_region_info(voxel_vertex_array_t* vertex_array, staging_t* staging, voxel_region_render_info_t* render_info, voxel_region_allocation_info_t* allocation_info) {
    uint32_t num_vertices = vertex_array->num_vertices;
    if (num_vertices == 0) {
        return result_success;
    }

    render_info->num_vertices = num_vertices;

    uint32_t num_array_bytes = num_vertices*sizeof(voxel_vertex_t);

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_STAGING_BUFFER,
        .size = num_array_bytes
    }, &staging_allocation_create_info, &staging->buffer, &staging->allocation, NULL) != VK_SUCCESS) {
        return result_failure;
    }

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_VERTEX_BUFFER,
        .size = num_array_bytes
    }, &device_allocation_create_info, &render_info->vertex_buffer, &allocation_info->vertex_allocation, NULL) != VK_SUCCESS) {
        return result_failure;
    }

    uint32_t num_chunks = div_ceil_uint32(num_vertices, NUM_VOXEL_VERTEX_CHUNK_MEMBERS); // Integer ceiling division
    uint32_t num_last_chunk_vertices = num_vertices % NUM_VOXEL_VERTEX_CHUNK_MEMBERS;
    if (num_last_chunk_vertices == 0) {
        num_last_chunk_vertices = NUM_VOXEL_VERTEX_CHUNK_MEMBERS;
    }

    void* mapped_data;
    if (vmaMapMemory(allocator, staging->allocation, &mapped_data) != VK_SUCCESS) {
        return result_failure;
    }

    voxel_vertex_chunk_t* chunk = vertex_array->chunk;
    for (uint32_t j = 0; j < num_chunks; j++) {
        uint32_t num_chunk_vertices = j == num_chunks - 1u ? num_last_chunk_vertices : NUM_VOXEL_VERTEX_CHUNK_MEMBERS;
        memcpy(mapped_data + j*sizeof(chunk->vertices), chunk->vertices, num_chunk_vertices*sizeof(voxel_vertex_t));

        voxel_vertex_chunk_t* new_chunk = chunk->next;
        free(chunk);
        chunk = new_chunk;
    }

    vmaUnmapMemory(allocator, staging->allocation);

    return result_success;
}
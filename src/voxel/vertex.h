#pragma once
#include "bitmask.h"
#include "face.h"
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#define NUM_TEXTURE_ARRAY_INDEX_BITS 8
#define NUM_FACE_BITS 3
#define NUM_VOXEL_REGION_AXIS_BITS 6

typedef uint32_t voxel_vertex_t;

typedef struct {
    uint8_t texture_array_index;
    voxel_face_t face;
    struct {
        uint8_t x;
        uint8_t y;
        uint8_t z;
    } position;
} voxel_vertex_create_info_t;

inline voxel_vertex_t create_voxel_vertex(voxel_vertex_create_info_t info) {
    return 0 |
        info.texture_array_index |
        (info.face << NUM_TEXTURE_ARRAY_INDEX_BITS) |
        (info.position.x << (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_BITS)) |
        (info.position.y << (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_BITS + 1*NUM_VOXEL_REGION_AXIS_BITS)) |
        (info.position.z << (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_BITS + 2*NUM_VOXEL_REGION_AXIS_BITS));
}

static_assert(sizeof(voxel_vertex_t) == sizeof(uint32_t), "Voxel vertex must be a 4 bytes long");
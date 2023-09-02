#pragma once
#include "bitmask.h"
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#define NUM_TEXTURE_ARRAY_INDEX_BITS 8
#define NUM_FACE_TYPE_BITS 3
#define NUM_VOXEL_REGION_AXIS_BITS 6

typedef uint32_t voxel_face_instance_t;

typedef struct {
    uint8_t texture_array_index;
    uint8_t face_type;
    struct {
        uint8_t x;
        uint8_t y;
        uint8_t z;
    } position;
} voxel_face_instance_create_info_t;

inline voxel_face_instance_t create_voxel_face_instance(voxel_face_instance_create_info_t info) {
    return 0 |
        info.texture_array_index |
        (info.face_type << NUM_TEXTURE_ARRAY_INDEX_BITS) |
        (info.position.x << (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_TYPE_BITS)) |
        (info.position.y << (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_TYPE_BITS + 1*NUM_VOXEL_REGION_AXIS_BITS)) |
        (info.position.z << (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_TYPE_BITS + 2*NUM_VOXEL_REGION_AXIS_BITS));
}

static_assert(sizeof(voxel_face_instance_t) == sizeof(uint32_t), "Voxel face instance must be a 4 bytes long");
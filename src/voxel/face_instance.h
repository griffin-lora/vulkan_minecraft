#pragma once
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

typedef union {
    uint32_t info;
    struct {
        struct {
            uint8_t x;
            uint8_t y;
            uint8_t z;
        } position;
        uint8_t texture_array_index;
    };
} voxel_face_instance_t;

static_assert(sizeof(voxel_face_instance_t) == sizeof(uint32_t), "Voxel face instance must be a 4 bytes long");
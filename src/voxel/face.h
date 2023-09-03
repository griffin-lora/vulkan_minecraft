#pragma once
#include <stdint.h>

#define NUM_CUBE_VOXEL_FACE_VERTICES 6

#define CUBE_VOXEL_TYPE_BEGIN 0
#define CUBE_VOXEL_TYPE_END 6

enum {
    voxel_face_front,
    voxel_face_back,
    voxel_face_top,
    voxel_face_bottom,
    voxel_face_right,
    voxel_face_left
};

typedef uint8_t voxel_face_t;
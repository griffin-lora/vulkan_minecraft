#pragma once

// #define NUM_CUBE_VOXEL_FACE_TYPES 6

#define CUBE_VOXEL_UNION(TYPE)\
    struct {\
        TYPE front;\
        TYPE back;\
        TYPE top;\
        TYPE bottom;\
        TYPE right;\
        TYPE left;\
    };\
    TYPE faces[NUM_VOXEL_FACE_TYPES];

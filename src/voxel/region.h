#pragma once
#include "type.h"
#include "face_instance.h"
#include "infos.h"
#include "result.h"
#include "vk/gfx_core.h"

#define VOXEL_REGION_SIZE 32

typedef struct {
    // types[x][y][z]
    voxel_type_t types[VOXEL_REGION_SIZE][VOXEL_REGION_SIZE][VOXEL_REGION_SIZE];
} voxel_region_voxel_type_array_t;

typedef struct {
    voxel_region_voxel_type_array_t* center;
    voxel_region_voxel_type_array_t* front;
    voxel_region_voxel_type_array_t* back;
    voxel_region_voxel_type_array_t* top;
    voxel_region_voxel_type_array_t* bottom;
    voxel_region_voxel_type_array_t* right;
    voxel_region_voxel_type_array_t* left;
} voxel_region_voxel_type_arrays_t;

#define NUM_VOXEL_FACE_INSTANCE_CHUNK_MEMBERS 4096

typedef struct voxel_face_instance_chunk voxel_face_instance_chunk_t;

struct voxel_face_instance_chunk {
    voxel_face_instance_t face_instances[NUM_VOXEL_FACE_INSTANCE_CHUNK_MEMBERS];
    voxel_face_instance_chunk_t* next;
};

typedef struct {
    uint32_t num_instances;
    voxel_face_instance_chunk_t* chunk;
} voxel_face_instance_array_t;

typedef struct {
    voxel_face_instance_array_t arrays[NUM_VOXEL_FACE_TYPES];
} voxel_face_instance_arrays_t;

void create_voxel_region_voxel_type_array(size_t region_x, size_t region_y, size_t region_z, voxel_region_voxel_type_array_t* voxel_types);
void create_voxel_face_instance_arrays(const voxel_region_voxel_type_arrays_t* voxel_type_arrays, voxel_face_instance_arrays_t* instance_arrays);
result_t begin_voxel_region_info(voxel_face_instance_arrays_t* instance_arrays, voxel_region_staging_t* staging, voxel_region_render_info_t* render_info, voxel_region_allocation_info_t* allocation_info);
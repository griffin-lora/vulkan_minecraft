#pragma once
#include "type.h"
#include "vertex.h"
#include "infos.h"
#include "result.h"
#include "vk/gfx_core.h"

#define VOXEL_REGION_SIZE (1 << NUM_VOXEL_REGION_AXIS_BITS)

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

#define NUM_VOXEL_VERTEX_CHUNK_MEMBERS 4096

typedef struct voxel_vertex_chunk voxel_vertex_chunk_t;

struct voxel_vertex_chunk {
    voxel_vertex_t vertices[NUM_VOXEL_VERTEX_CHUNK_MEMBERS];
    voxel_vertex_chunk_t* next;
};

typedef struct {
    uint32_t num_vertices;
    voxel_vertex_chunk_t* chunk;
} voxel_vertex_array_t;

void create_voxel_region_voxel_type_array(size_t region_x, size_t region_y, size_t region_z, voxel_region_voxel_type_array_t* voxel_types);
void create_voxel_vertex_array(const voxel_region_voxel_type_arrays_t* voxel_type_arrays, voxel_vertex_array_t* vertex_array);

result_t begin_voxel_region_info(voxel_vertex_array_t* vertex_array, staging_t* staging, voxel_region_render_info_t* render_info, voxel_region_allocation_info_t* allocation_info);
#pragma once
#include "vk/gfx_core.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/vec3.h>

// #define NUM_VOXEL_FACE_TYPES 6

typedef struct {
    vec3s position;
    uint32_t num_vertices;
    VkBuffer vertex_buffer;
} voxel_region_render_info_t;

typedef struct {
    VmaAllocation vertex_allocation;
} voxel_region_allocation_info_t;
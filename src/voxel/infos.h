#pragma once
#include "vk.h"
#include "vk/gfx_core.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/vec3.h>

// Array of instances = model

#define NUM_VOXEL_FACE_TYPES 6

typedef struct {
    VkBuffer vertex_buffer;
    VkBuffer index_buffer;
    uint32_t num_indices;
} voxel_face_type_render_info_t;

typedef union {
    struct {
        VmaAllocation vertex_allocation;
        VmaAllocation index_allocation;
    };
    VmaAllocation allocations[2];
} voxel_face_type_allocation_info_t;

typedef struct {
    VkBuffer instance_buffer;
    uint32_t num_instances;
} voxel_face_model_render_info_t;

typedef struct {
    VmaAllocation instance_allocation;
} voxel_face_model_allocation_info_t;

typedef struct {
    vec3s position;
} voxel_region_render_info_t;
#pragma once
#include "vk.h"
#include "mesh.h"
#include "voxel/cube.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>

typedef struct {
    VkBuffer vertex_buffer;
    VkBuffer index_buffer;
    VkBuffer instance_buffer;
    uint32_t num_indices;
    uint32_t num_instances;
} voxel_face_render_info_t;

typedef union {
    struct {
        VmaAllocation vertex_allocation;
        VmaAllocation index_allocation;
        VmaAllocation instance_allocation;
    };
    VmaAllocation allocations[3];
} voxel_face_allocation_info_t;

typedef union {
    CUBE_VOXEL_UNION(voxel_face_render_info_t)
} cube_voxel_render_info_t;

typedef union {
    CUBE_VOXEL_UNION(voxel_face_allocation_info_t)
} cube_voxel_allocation_info_t;

extern cube_voxel_render_info_t cube_voxel_render_info;
extern cube_voxel_allocation_info_t cube_voxel_allocation_info;

#define NUM_TEXTURE_IMAGES 1
#define NUM_TEXTURE_LAYERS 2
extern VkSampler texture_image_sampler;
extern VkImage texture_images[NUM_TEXTURE_IMAGES];
extern VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
extern VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

const char* init_vulkan_assets(const VkPhysicalDeviceProperties* physical_device_properties);
void term_vulkan_assets(void);
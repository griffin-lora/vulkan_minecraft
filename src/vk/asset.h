#pragma once
#include "vk.h"
#include "mesh.h"
#include "voxel/cube.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>
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
    voxel_face_model_render_info_t face_model_infos[NUM_VOXEL_FACE_TYPES];
} voxel_region_render_info_t;

typedef struct {
    voxel_face_model_allocation_info_t face_model_infos[NUM_VOXEL_FACE_TYPES];
} voxel_region_allocation_info_t;

extern voxel_face_type_render_info_t voxel_face_type_render_infos[NUM_VOXEL_FACE_TYPES];
extern voxel_face_type_allocation_info_t voxel_face_type_allocation_infos[NUM_VOXEL_FACE_TYPES];

extern voxel_region_render_info_t voxel_region_render_info;
extern voxel_region_allocation_info_t voxel_region_allocation_info;

#define NUM_TEXTURE_IMAGES 1
#define NUM_TEXTURE_LAYERS 2
extern VkSampler texture_image_sampler;
extern VkImage texture_images[NUM_TEXTURE_IMAGES];
extern VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
extern VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

const char* init_vulkan_assets(const VkPhysicalDeviceProperties* physical_device_properties);
void term_vulkan_assets(void);
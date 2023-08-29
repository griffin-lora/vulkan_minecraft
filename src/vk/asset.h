#pragma once
#include "vk.h"
#include "mesh.h"
#include "voxel/infos.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>

extern voxel_face_type_render_info_t voxel_face_type_render_infos[NUM_VOXEL_FACE_TYPES];
extern voxel_face_type_allocation_info_t voxel_face_type_allocation_infos[NUM_VOXEL_FACE_TYPES];

extern voxel_region_render_info_t voxel_region_render_info;
extern voxel_region_allocation_info_t voxel_region_allocation_info;

#define NUM_TEXTURE_IMAGES 1
#define NUM_TEXTURE_LAYERS 6
extern VkSampler texture_image_sampler;
extern VkImage texture_images[NUM_TEXTURE_IMAGES];
extern VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
extern VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

const char* init_vulkan_assets(const VkPhysicalDeviceProperties* physical_device_properties);
void term_vulkan_assets(void);
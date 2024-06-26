#pragma once
#include "result.h"
#include <vk_mem_alloc.h>
#include <cglm/struct/mat4.h>

#define NUM_TEXTURE_IMAGES 2
#define NUM_TEXTURE_LAYERS 6
extern VkImage texture_images[NUM_TEXTURE_IMAGES];
extern VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
extern VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

result_t init_assets(const VkPhysicalDeviceProperties* physical_device_properties);
void term_assets(void);
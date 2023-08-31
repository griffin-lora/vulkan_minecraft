#pragma once
#include "vk.h"
#include "mesh.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>

#define NUM_TEXTURE_IMAGES 1
#define NUM_TEXTURE_LAYERS 6
extern VkSampler voxel_texture_image_sampler;
extern VkImage texture_images[NUM_TEXTURE_IMAGES];
extern VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
extern VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

#define TEXT_GLYPH_SIZE 16

#define FPS_TEXT_MODEL_INDEX 0
#define NUM_FPS_TEXT_MODEL_GLYPHS 32

const char* init_assets(const VkPhysicalDeviceProperties* physical_device_properties);
void term_assets(void);
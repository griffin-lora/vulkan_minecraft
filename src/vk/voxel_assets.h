#pragma once
#include "result.h"
#include "vk/core.h"

extern VkSampler voxel_region_texture_image_sampler;

const char* begin_voxel_assets(float max_anistropy, uint32_t num_mip_levels);
void transfer_voxel_assets(VkCommandBuffer command_buffer);
void end_voxel_assets(void);
void term_voxel_assets();
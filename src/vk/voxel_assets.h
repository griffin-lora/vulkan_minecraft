#pragma once
#include "result.h"
#include "mesh.h"
#include "voxel/infos.h"

extern voxel_face_type_render_info_t voxel_face_type_render_infos[NUM_VOXEL_FACE_TYPES];
extern voxel_face_type_allocation_info_t voxel_face_type_allocation_infos[NUM_VOXEL_FACE_TYPES];

#define NUM_VOXEL_REGIONS 64

extern voxel_region_render_info_t voxel_region_render_infos[NUM_VOXEL_REGIONS];
extern voxel_region_allocation_info_t voxel_region_allocation_infos[NUM_VOXEL_REGIONS];

extern VkSampler voxel_region_texture_image_sampler;

const char* begin_voxel_assets(float max_anistropy, uint32_t num_mip_levels);
void transfer_voxel_assets(VkCommandBuffer command_buffer);
void end_voxel_assets(void);
void term_voxel_assets();
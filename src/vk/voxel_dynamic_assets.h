#pragma once
#include "voxel/infos.h"
#include "voxel/region.h"
#include "dynamic_asset_transfer.h"

#define NUM_VOXEL_REGIONS 64
#define NUM_XZ_VOXEL_REGIONS 8

#define NUM_QUEUED_VOXEL_DYNAMIC_ASSSETS 2

extern dynamic_assets_front_index_t voxel_dynamic_assets_front_index;
extern voxel_region_render_info_t voxel_region_render_info_arrays[NUM_QUEUED_VOXEL_DYNAMIC_ASSSETS][NUM_VOXEL_REGIONS];
extern voxel_region_allocation_info_t voxel_region_allocation_info_arrays[NUM_QUEUED_VOXEL_DYNAMIC_ASSSETS][NUM_VOXEL_REGIONS];

result_t begin_voxel_region_info(voxel_vertex_array_t* vertex_array, staging_t* staging, voxel_region_render_info_t* render_info, voxel_region_allocation_info_t* allocation_info);
void transfer_voxel_region_info(const staging_t* staging, const voxel_region_render_info_t* render_info);
void end_voxel_region_info(const staging_t* staging);

void destroy_voxel_dynamic_assets(void);
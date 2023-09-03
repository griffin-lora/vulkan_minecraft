#pragma once
#include "dynamic_assets.h"
#include "voxel/infos.h"

#define NUM_VOXEL_REGIONS 64
#define NUM_XZ_VOXEL_REGIONS 8

#define NUM_QUEUED_VOXEL_DYNAMIC_ASSSETS 2

extern dynamic_assets_front_index_t voxel_dynamic_assets_front_index;
extern voxel_region_render_info_t voxel_region_render_info_arrays[NUM_QUEUED_VOXEL_DYNAMIC_ASSSETS][NUM_VOXEL_REGIONS];
extern voxel_region_allocation_info_t voxel_region_allocation_info_arrays[NUM_QUEUED_VOXEL_DYNAMIC_ASSSETS][NUM_VOXEL_REGIONS];

result_t begin_voxel_regions(void);
void transfer_voxel_regions(void);
void end_voxel_regions(void);

void term_voxel_dynamic_assets(void);
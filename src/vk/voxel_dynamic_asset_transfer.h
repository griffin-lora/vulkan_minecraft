#pragma once
#include "voxel/infos.h"
#include "voxel/region.h"

result_t begin_voxel_region_info(voxel_vertex_array_t* vertex_array, staging_t* staging, voxel_region_render_info_t* render_info, voxel_region_allocation_info_t* allocation_info);
void transfer_voxel_region_info(const staging_t* staging, const voxel_region_render_info_t* render_info);
void end_voxel_region_info(const staging_t* staging);

void destroy_voxel_region_info(voxel_region_render_info_t* render_info, voxel_region_allocation_info_t* allocation_info);
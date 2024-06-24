#include "voxel_dynamic_asset_transfer.h"
#include "dynamic_asset_transfer.h"
#include "result.h"
#include "voxel/region.h"
#include "vk/core.h"
#include "vk/defaults.h"
#include "util.h"
#include <string.h>

result_t begin_voxel_region_info(voxel_vertex_array_t* vertex_array, staging_t* staging, voxel_region_render_info_t* render_info, voxel_region_allocation_info_t* allocation_info) {
    uint32_t num_vertices = vertex_array->num_vertices;
    if (num_vertices == 0) {
        return result_success;
    }

    render_info->num_vertices = num_vertices;

    uint32_t num_array_bytes = num_vertices*sizeof(voxel_vertex_t);

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_STAGING_BUFFER,
        .size = num_array_bytes
    }, &staging_allocation_create_info, &staging->buffer, &staging->allocation, NULL) != VK_SUCCESS) {
        return result_buffer_create_failure;
    }

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_VERTEX_BUFFER,
        .size = num_array_bytes
    }, &device_allocation_create_info, &render_info->vertex_buffer, &allocation_info->vertex_allocation, NULL) != VK_SUCCESS) {
        return result_buffer_create_failure;
    }

    uint32_t num_chunks = div_ceil_uint32(num_vertices, NUM_VOXEL_VERTEX_CHUNK_MEMBERS); // Integer ceiling division
    uint32_t num_last_chunk_vertices = num_vertices % NUM_VOXEL_VERTEX_CHUNK_MEMBERS;
    if (num_last_chunk_vertices == 0) {
        num_last_chunk_vertices = NUM_VOXEL_VERTEX_CHUNK_MEMBERS;
    }

    void* mapped_data;
    if (vmaMapMemory(allocator, staging->allocation, &mapped_data) != VK_SUCCESS) {
        return result_memory_map_failure;
    }

    voxel_vertex_chunk_t* chunk = vertex_array->chunk;
    for (uint32_t j = 0; j < num_chunks; j++) {
        uint32_t num_chunk_vertices = j == num_chunks - 1u ? num_last_chunk_vertices : NUM_VOXEL_VERTEX_CHUNK_MEMBERS;
        memcpy(mapped_data + j*sizeof(chunk->vertices), chunk->vertices, num_chunk_vertices*sizeof(voxel_vertex_t));

        voxel_vertex_chunk_t* new_chunk = chunk->next;
        free(chunk);
        chunk = new_chunk;
    }

    vmaUnmapMemory(allocator, staging->allocation);

    return result_success;
}

void transfer_voxel_region_info(const staging_t* staging, const voxel_region_render_info_t* render_info) {
    if (render_info->num_vertices != 0) {
        vkCmdCopyBuffer(dynamic_asset_transfer_command_buffer, staging->buffer, render_info->vertex_buffer, 1, &(VkBufferCopy) {
            .size = sizeof(voxel_vertex_t)*render_info->num_vertices
        });
    }
}

void end_voxel_region_info(const staging_t* staging) {
    vmaDestroyBuffer(allocator, staging->buffer, staging->allocation);
}

void destroy_voxel_region_info(voxel_region_render_info_t* render_info, voxel_region_allocation_info_t* allocation_info) {
    if (render_info->vertex_buffer == NULL) {
        return;
    }

    vmaDestroyBuffer(allocator, render_info->vertex_buffer, allocation_info->vertex_allocation);
    render_info->vertex_buffer = NULL;
}
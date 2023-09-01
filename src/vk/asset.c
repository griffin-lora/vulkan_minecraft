#include "asset.h"
#include "util.h"
#include "mesh.h"
#include "core.h"
#include "gfx_core.h"
#include "voxel_color_pipeline.h"
#include "text_pipeline.h"
#include "defaults.h"
#include "voxel_assets.h"
#include "voxel/face_instance.h"
#include "voxel/region.h"
#include <malloc.h>
#include <string.h>
#include <stdalign.h>
#include <stb_image.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/cam.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine.h>

alignas(64)
VkImage texture_images[NUM_TEXTURE_IMAGES];
VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

uint32_t text_glyph_image_width;
uint32_t text_glyph_image_height;

const char* init_assets(const VkPhysicalDeviceProperties* physical_device_properties) {
    struct {
        const char* path;
        int channels;
    } image_load_infos[][NUM_TEXTURE_LAYERS] = {
        {
            { "image/cube_voxel_0.png", STBI_rgb },
            { "image/cube_voxel_1.png", STBI_rgb },
            { "image/cube_voxel_2.png", STBI_rgb },
            { "image/cube_voxel_3.png", STBI_rgb },
            { "image/cube_voxel_4.png", STBI_rgb },
            { "image/cube_voxel_5.png", STBI_rgb }
        },
        {
            { "image/font.png", STBI_rgb_alpha }
        }
    };

    void* pixel_arrays[NUM_TEXTURE_IMAGES][NUM_TEXTURE_LAYERS];

    image_create_info_t image_create_infos[NUM_TEXTURE_IMAGES] = {
        {
            .num_pixel_bytes = 3,
            .info = {
                DEFAULT_VK_SAMPLED_IMAGE,
                .format = VK_FORMAT_R8G8B8_SRGB,
                .arrayLayers = NUM_TEXTURE_LAYERS
            }
        },
        {
            .num_pixel_bytes = 4,
            .info = {
                DEFAULT_VK_SAMPLED_IMAGE,
                .format = VK_FORMAT_R8G8B8A8_SRGB,
                .arrayLayers = 1
            }
        }
    };

    for (size_t i = 0; i < NUM_TEXTURE_IMAGES; i++) {
        uint32_t width;
        uint32_t height;

        image_create_info_t* info = &image_create_infos[i];
        info->pixel_arrays = (void**)&pixel_arrays[i];
        
        for (size_t j = 0; j < info->info.arrayLayers; j++) {
            int new_width;
            int new_height;
            info->pixel_arrays[j] = stbi_load(image_load_infos[i][j].path, &new_width, &new_height, (int[1]) { 0 }, image_load_infos[i][j].channels);

            if (info->pixel_arrays[j] == NULL) {
                return "Failed to load image pixels\n";
            }

            if (j > 0 && ((uint32_t)new_width != width || (uint32_t)new_width != height)) {
                return "Failed to get image in same size\n";
            }

            width = (uint32_t)new_width;
            height = (uint32_t)new_height;

            info->info.extent.width = width;
            info->info.extent.height = height;
            info->info.mipLevels = ((uint32_t)floorf(log2f((float)max_uint32(width, height)))) + 1;
        }
    }

    staging_t image_stagings[NUM_TEXTURE_IMAGES];

    if (begin_images(NUM_TEXTURE_IMAGES, image_create_infos, image_stagings, texture_images, texture_image_allocations) != result_success) {
        return "Failed to begin creating images\n";
    }

    for (size_t i = 0; i < NUM_TEXTURE_IMAGES; i++) {
        const image_create_info_t* info = &image_create_infos[i];
        
        for (size_t j = 0; j < info->info.arrayLayers; j++) {
            stbi_image_free(info->pixel_arrays[j]);
        }
    }

    text_glyph_image_width = image_create_infos[1].info.extent.width;
    text_glyph_image_height = image_create_infos[1].info.extent.height;
    
    text_glyph_vertex_t text_glyph_vertices[] = {
        { {{ 0.0f, 0.0f }}, {{ 0.0f, 0.0f }} },
        { {{ 1.0f/(float)TEXT_GLYPH_SIZE, 0.0f }}, {{ (float)TEXT_GLYPH_SIZE/(float)text_glyph_image_width, 0.0f }} },
        { {{ 1.0f/(float)TEXT_GLYPH_SIZE, 1.0f/(float)TEXT_GLYPH_SIZE }}, {{ (float)TEXT_GLYPH_SIZE/(float)text_glyph_image_width, (float)TEXT_GLYPH_SIZE/(float)text_glyph_image_height }} },
        { {{ 0.0f, 1.0f/(float)TEXT_GLYPH_SIZE }}, {{ 0.0f, (float)TEXT_GLYPH_SIZE/(float)text_glyph_image_height }} }
    };
    
    uint16_t text_glyph_indices[] = {
        0, 1, 2, 2, 3, 0
    };
    
    uint32_t num_text_glyph_vertices = NUM_ELEMS(text_glyph_vertices);
    uint32_t num_text_glyph_indices = NUM_ELEMS(text_glyph_indices);

    text_glyph_render_info.num_indices = num_text_glyph_indices;

    staging_t text_glyph_vertex_staging;
    staging_t text_glyph_index_staging;

    uint32_t num_text_glyph_vertex_bytes = sizeof(text_glyph_vertex_t);
    uint32_t num_text_glyph_index_bytes = sizeof(uint16_t);

    const char* msg = begin_voxel_assets(physical_device_properties->limits.maxSamplerAnisotropy, image_create_infos[0].info.mipLevels);
    if (msg != NULL) { return msg; }

    if (begin_buffer(&vertex_buffer_create_info, num_text_glyph_vertices, num_text_glyph_vertex_bytes, text_glyph_vertices, &text_glyph_vertex_staging, &text_glyph_render_info.vertex_buffer, &text_glyph_allocation_info.vertex_allocation) != result_success) {
        return "Failed to begin creating vertex buffer\n";
    }

    if (begin_buffer(&index_buffer_create_info, num_text_glyph_indices, num_text_glyph_index_bytes, text_glyph_indices, &text_glyph_index_staging, &text_glyph_render_info.index_buffer, &text_glyph_allocation_info.index_allocation) != result_success) {
        return "Failed to begin creating index buffer\n";
    }

    if (init_text_model(FPS_TEXT_MODEL_INDEX, (vec2s) {{ 0.0f, 0.0f }}, NUM_FPS_TEXT_MODEL_GLYPHS) != result_success) {
        return "Failed to create text model\n";
    }

    //

    VkFence transfer_fence;
    if (vkCreateFence(device, &(VkFenceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    }, NULL, &transfer_fence) != VK_SUCCESS) {
        return "Failed to create transfer fence\n";
    }

    VkCommandBuffer command_buffer;
    if (vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = command_pool // TODO: Use separate command pool
    }, &command_buffer) != VK_SUCCESS) {
        return "Failed to create transfer command buffer\n";
    }

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }) != VK_SUCCESS) {
        return "Failed to write to transfer command buffer\n";
    }

    transfer_images(command_buffer, NUM_TEXTURE_IMAGES, image_create_infos, image_stagings, texture_images);

    transfer_voxel_assets(command_buffer);

    vkCmdCopyBuffer(command_buffer, text_glyph_vertex_staging.buffer, text_glyph_render_info.vertex_buffer, 1, &(VkBufferCopy) {
        .size = num_text_glyph_vertex_bytes*num_text_glyph_vertices
    });
    vkCmdCopyBuffer(command_buffer, text_glyph_index_staging.buffer, text_glyph_render_info.index_buffer, 1, &(VkBufferCopy) {
        .size = num_text_glyph_index_bytes*num_text_glyph_indices
    });

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return "Failed to write to transfer command buffer\n";
    }

    vkQueueSubmit(graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer
    }, transfer_fence);
    vkWaitForFences(device, 1, &transfer_fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device, transfer_fence, NULL);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);

    end_images(NUM_TEXTURE_IMAGES, image_stagings);

    end_voxel_assets();

    vmaDestroyBuffer(allocator, text_glyph_vertex_staging.buffer, text_glyph_vertex_staging.allocation);
    vmaDestroyBuffer(allocator, text_glyph_index_staging.buffer, text_glyph_index_staging.allocation);

    //

    for (size_t i = 0; i < NUM_TEXTURE_IMAGES; i++) {
        const VkImageCreateInfo* image_create_info = &image_create_infos[i].info;

        if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
            DEFAULT_VK_IMAGE_VIEW,
            .image = texture_images[i],
            .viewType = image_create_info->arrayLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
            .format = image_create_info->format,
            .subresourceRange.levelCount = image_create_info->mipLevels,
            .subresourceRange.layerCount = image_create_info->arrayLayers,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        }, NULL, &texture_image_views[i]) != VK_SUCCESS) {
            return "Failed to create texture image view\n";
        }
    }

    return NULL;
}

void term_assets(void) {
    destroy_images(NUM_TEXTURE_IMAGES, texture_images, texture_image_allocations, texture_image_views);

    term_voxel_assets();

    vmaDestroyBuffer(allocator, text_glyph_render_info.vertex_buffer, text_glyph_allocation_info.vertex_allocation);
    vmaDestroyBuffer(allocator, text_glyph_render_info.index_buffer, text_glyph_allocation_info.index_allocation);
}
#include "asset.h"
#include "result.h"
#include "util.h"
#include "core.h"
#include "gfx_core.h"
#include "defaults.h"
#include "voxel_assets.h"
#include "text_assets.h"
#include "dynamic_asset_transfer.h"
#include <malloc.h>
#include <string.h>
#include <stdalign.h>
#include <stb/stb_image.h>
#include <cglm/struct/cam.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine.h>

alignas(64)
VkImage texture_images[NUM_TEXTURE_IMAGES];
VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

result_t init_assets(const VkPhysicalDeviceProperties* physical_device_properties) {
    result_t result;

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
                return result_image_pixels_load_failure;
            }

            if (j > 0 && ((uint32_t)new_width != width || (uint32_t)new_width != height)) {
                return result_image_dimensions_invalid;
            }

            width = (uint32_t)new_width;
            height = (uint32_t)new_height;

            info->info.extent.width = width;
            info->info.extent.height = height;
            info->info.mipLevels = ((uint32_t)floorf(log2f((float)max_uint32(width, height)))) + 1;
        }
    }

    staging_t image_stagings[NUM_TEXTURE_IMAGES];

    if ((result = begin_images(NUM_TEXTURE_IMAGES, image_create_infos, image_stagings, texture_images, texture_image_allocations)) != result_success) {
        return result;
    }

    for (size_t i = 0; i < NUM_TEXTURE_IMAGES; i++) {
        const image_create_info_t* info = &image_create_infos[i];
        
        for (size_t j = 0; j < info->info.arrayLayers; j++) {
            stbi_image_free(info->pixel_arrays[j]);
        }
    }

    if ((result = begin_voxel_assets(physical_device_properties->limits.maxSamplerAnisotropy, image_create_infos[0].info.mipLevels)) != result_success) {
        return result;
    }
    if ((result = begin_text_assets(physical_device_properties->limits.maxSamplerAnisotropy, image_create_infos[1].info.mipLevels, image_create_infos[1].info.extent.width, image_create_infos[1].info.extent.height)) != result_success) {
        return result;
    }

    //

    VkFence transfer_fence;
    if (vkCreateFence(device, &(VkFenceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    }, NULL, &transfer_fence) != VK_SUCCESS) {
        return result_synchronization_primitive_create_failure;
    }

    VkCommandBuffer command_buffer;
    if (vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = dynamic_asset_transfer_command_pool
    }, &command_buffer) != VK_SUCCESS) {
        return result_command_buffers_allocate_failure;
    }

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    transfer_images(command_buffer, NUM_TEXTURE_IMAGES, image_create_infos, image_stagings, texture_images);

    transfer_voxel_assets(command_buffer);
    transfer_text_assets(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return result_command_buffer_end_failure;
    }

    pthread_mutex_lock(&queue_submit_mutex);
    vkQueueSubmit(graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer
    }, transfer_fence);
    pthread_mutex_unlock(&queue_submit_mutex);

    vkWaitForFences(device, 1, &transfer_fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device, transfer_fence, NULL);

    vkFreeCommandBuffers(device, dynamic_asset_transfer_command_pool, 1, &command_buffer);

    end_images(NUM_TEXTURE_IMAGES, image_stagings);

    end_voxel_assets();
    end_text_assets();

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
            return result_image_view_create_failure;
        }
    }

    return result_success;
}

void term_assets(void) {
    destroy_images(NUM_TEXTURE_IMAGES, texture_images, texture_image_allocations, texture_image_views);

    term_voxel_assets();
    term_text_assets();
}
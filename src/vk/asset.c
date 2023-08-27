#include "asset.h"
#include "util.h"
#include "mesh.h"
#include "core.h"
#include "gfx_core.h"
#include "color_pipeline.h"
#include "defaults.h"
#include "voxel/face.h"
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

VkBuffer vertex_buffer_arrays[NUM_MODELS][NUM_VERTEX_ARRAYS];
VmaAllocation vertex_buffer_allocation_arrays[NUM_MODELS][NUM_VERTEX_ARRAYS];

VkBuffer index_buffers[NUM_MODELS];
VmaAllocation index_buffer_allocations[NUM_MODELS];

VkBuffer instance_buffers[NUM_MODELS];
VmaAllocation instance_buffer_allocations[NUM_MODELS];

uint32_t num_indices_array[NUM_MODELS];
uint32_t num_instances_array[NUM_MODELS];

VkSampler texture_image_sampler;
VkImage texture_images[NUM_TEXTURE_IMAGES];
VmaAllocation texture_image_allocations[NUM_TEXTURE_IMAGES];
VkImageView texture_image_views[NUM_TEXTURE_IMAGES];

const char* init_vulkan_assets(const VkPhysicalDeviceProperties* physical_device_properties) {
    struct {
        const char* path;
        int channels;
    } image_load_infos[][NUM_TEXTURE_LAYERS] = {
        {
            { "image/cube_voxel_0.png", STBI_rgb },
            { "image/cube_voxel_1.png", STBI_rgb }
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
                return "Pixels \n";
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

    const char* mesh_paths[] = {
        "mesh/cube_voxel_top_face.gltf"
    };

    face_t faces[64];
    {
        size_t i = 0;
        for (float x = 0.0f; x < 8.0f; x++) {
            for (float y = 0.0f; y < 8.0f; y++, i++) {
                faces[i] = (face_t) {
                    .position = {{ x * 4.0f, 0.0f, y * 4.0f }}
                };
            }
        }
    }

    num_instances_array[0] = NUM_ELEMS(faces);
    void* model_matrix_arrays[] = {
        faces
    };

    uint32_t num_vertices_array[NUM_MODELS];

    staging_t vertex_staging_arrays[NUM_MODELS][NUM_VERTEX_ARRAYS];
    staging_t index_stagings[NUM_MODELS];
    staging_t instance_stagings[NUM_MODELS];

    uint32_t num_index_bytes = sizeof(uint16_t);
    uint32_t num_instance_bytes = sizeof(face_t);

    for (size_t i = 0; i < NUM_MODELS; i++) {
        mesh_t mesh;
        if (load_gltf_mesh(mesh_paths[i], &mesh) != result_success) {
            return "Failed to load mesh\n";
        }

        num_vertices_array[i] = mesh.num_vertices;
        num_indices_array[i] = mesh.num_indices;

        if (begin_buffers(mesh.num_vertices, &vertex_buffer_create_info, NUM_VERTEX_ARRAYS, &mesh.vertex_arrays[0].data, num_vertex_bytes_array, vertex_staging_arrays[i], vertex_buffer_arrays[i], vertex_buffer_allocation_arrays[i]) != result_success) {
            return "Failed to begin creating vertex buffers\n"; 
        }

        if (begin_buffers(mesh.num_indices, &index_buffer_create_info, 1, &mesh.indices_data, &num_index_bytes, &index_stagings[i], &index_buffers[i], &index_buffer_allocations[i]) != result_success) {
            return "Failed to begin creating index buffer\n";
        }

        if (begin_buffers(num_instances_array[i], &vertex_buffer_create_info, 1, &model_matrix_arrays[i], &num_instance_bytes, &instance_stagings[i], &instance_buffers[i], &instance_buffer_allocations[i]) != result_success) {
            return "Failed to begin creating instance buffer\n";
        }

        for (size_t i = 0; i < NUM_VERTEX_ARRAYS; i++) {
            free(mesh.vertex_arrays[i].data);
        }
        free(mesh.indices_data);
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

    for (size_t i = 0; i < NUM_MODELS; i++) {
        transfer_buffers(command_buffer, num_vertices_array[i], NUM_VERTEX_ARRAYS, num_vertex_bytes_array, vertex_staging_arrays[i], vertex_buffer_arrays[i]);
        transfer_buffers(command_buffer, num_indices_array[i], 1, &num_index_bytes, &index_stagings[i], &index_buffers[i]);
        transfer_buffers(command_buffer, num_instances_array[i], 1, &num_instance_bytes, &instance_stagings[i], &instance_buffers[i]);
    }

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

    for (size_t i = 0; i < NUM_MODELS; i++) {
        end_buffers(NUM_VERTEX_ARRAYS, vertex_staging_arrays[i]);
        end_buffers(1, &index_stagings[i]);
        end_buffers(1, &instance_stagings[i]);
    }

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

    if (vkCreateSampler(device, &(VkSamplerCreateInfo) {
        DEFAULT_VK_SAMPLER,
        .minFilter = VK_FILTER_NEAREST,
        .magFilter = VK_FILTER_NEAREST,
        .anisotropyEnable = VK_FALSE,
        .maxLod = (float)image_create_infos[0].info.mipLevels
    }, NULL, &texture_image_sampler) != VK_SUCCESS) {
        return "Failed to create tetxure image sampler\n";
    }

    return NULL;
}

void term_vulkan_assets(void) {
    vkDestroySampler(device, texture_image_sampler, NULL);
    destroy_images(NUM_TEXTURE_IMAGES, texture_images, texture_image_allocations, texture_image_views);

    for (size_t i = 0; i < NUM_MODELS; i++) {
        for (size_t j = 0; j < NUM_VERTEX_ARRAYS; j++) {
            vmaDestroyBuffer(allocator, vertex_buffer_arrays[i][j], vertex_buffer_allocation_arrays[i][j]);
        }
        vmaDestroyBuffer(allocator, index_buffers[i], index_buffer_allocations[i]);
        vmaDestroyBuffer(allocator, instance_buffers[i], instance_buffer_allocations[i]);
    }
}
#include "asset.h"
#include "util.h"
#include "mesh.h"
#include "core.h"
#include "gfx_core.h"
#include "color_pipeline.h"
#include "defaults.h"
#include "voxel/face_instance.h"
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

cube_voxel_render_info_t cube_voxel_render_info = { 0 };
cube_voxel_allocation_info_t cube_voxel_allocation_info = { 0 };

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
        "mesh/cube_voxel.glb"
    };

    voxel_face_instance_t face_instances[64];
    {
        size_t i = 0;
        for (uint8_t x = 0; x < 8; x++) {
            for (uint8_t y = 0; y < 8; y++, i++) {
                face_instances[i] = (voxel_face_instance_t) {
                    .position = { x, 0, y }
                };
            }
        }
    }

    cube_voxel_render_info.top.num_instances = NUM_ELEMS(face_instances);
    union {
        CUBE_VOXEL_UNION(void*)
    } voxel_face_instances;
    voxel_face_instances.top = face_instances;

    uint32_t num_vertices_array[NUM_CUBE_VOXEL_FACES];

    staging_t vertex_stagings[NUM_CUBE_VOXEL_FACES];
    staging_t index_stagings[NUM_CUBE_VOXEL_FACES];
    staging_t instance_stagings[NUM_CUBE_VOXEL_FACES];

    uint32_t num_vertex_bytes = sizeof(voxel_vertex_t);
    uint32_t num_index_bytes = sizeof(uint16_t);
    uint32_t num_instance_bytes = sizeof(voxel_face_instance_t);

    voxel_face_mesh_t cube_face_meshes[NUM_CUBE_VOXEL_FACES];

    if (load_gltf_voxel_face_meshes(mesh_paths[0], NUM_CUBE_VOXEL_FACES, (const char*[]) { "Front", "Back", "Top", "Bottom", "Right", "Left" }, cube_face_meshes) != result_success) {
        return "Failed to load mesh\n";
    }

    for (size_t i = 0; i < NUM_CUBE_VOXEL_FACES; i++) {
        // cube_voxel_render_info.faces[i].num_instances = NUM_ELEMS(face_instances);
        // voxel_face_instances.faces[i] = face_instances;

        const voxel_face_mesh_t* mesh = &cube_face_meshes[i];
        voxel_face_render_info_t* render_info = &cube_voxel_render_info.faces[i];
        voxel_face_allocation_info_t* allocation_info = &cube_voxel_allocation_info.faces[i];

        num_vertices_array[i] = mesh->num_vertices;
        render_info->num_indices = mesh->num_indices;

        if (begin_buffers(mesh->num_vertices, &vertex_buffer_create_info, 1, &mesh->vertices_data, &num_vertex_bytes, &vertex_stagings[i], &render_info->vertex_buffer, &allocation_info->vertex_allocation) != result_success) {
            return "Failed to begin creating vertex buffers\n"; 
        }

        if (begin_buffers(mesh->num_indices, &index_buffer_create_info, 1, &mesh->indices_data, &num_index_bytes, &index_stagings[i], &render_info->index_buffer, &allocation_info->index_allocation) != result_success) {
            return "Failed to begin creating index buffer\n";
        }

        if (render_info->num_instances != 0) {
            if (begin_buffers(render_info->num_instances, &vertex_buffer_create_info, 1, &voxel_face_instances.faces[i], &num_instance_bytes, &instance_stagings[i], &render_info->instance_buffer, &allocation_info->instance_allocation) != result_success) {
                return "Failed to begin creating instance buffer\n";
            }
        }

        free(mesh->vertices_data);
        free(mesh->indices_data);
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

    for (size_t i = 0; i < NUM_CUBE_VOXEL_FACES; i++) {
        const voxel_face_render_info_t* render_info = &cube_voxel_render_info.faces[i];

        transfer_buffers(command_buffer, num_vertices_array[i], 1, &num_vertex_bytes, &vertex_stagings[i], &render_info->vertex_buffer);
        transfer_buffers(command_buffer, render_info->num_indices, 1, &num_index_bytes, &index_stagings[i], &render_info->index_buffer);
        if (render_info->num_instances != 0) {
            transfer_buffers(command_buffer, render_info->num_instances, 1, &num_instance_bytes, &instance_stagings[i], &render_info->instance_buffer);
        }
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

    for (size_t i = 0; i < NUM_CUBE_VOXEL_FACES; i++) {
        end_buffers(1, &vertex_stagings[i]);
        end_buffers(1, &index_stagings[i]);
        if (cube_voxel_render_info.faces[i].num_instances != 0) {
            end_buffers(1, &instance_stagings[i]);
        }
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

    for (size_t i = 0; i < NUM_CUBE_VOXEL_FACES; i++) {
        const voxel_face_render_info_t* render_info = &cube_voxel_render_info.faces[i];
        const voxel_face_allocation_info_t* allocation_info = &cube_voxel_allocation_info.faces[i];

        vmaDestroyBuffer(allocator, render_info->vertex_buffer, allocation_info->vertex_allocation);
        vmaDestroyBuffer(allocator, render_info->index_buffer, allocation_info->index_allocation);
        if (render_info->num_instances != 0) {
            vmaDestroyBuffer(allocator, render_info->instance_buffer, allocation_info->instance_allocation);
        }
    }
}
#include "asset.h"
#include "util.h"
#include "mesh.h"
#include "core.h"
#include "gfx_core.h"
#include "color_pipeline.h"
#include "defaults.h"
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
voxel_face_type_render_info_t voxel_face_type_render_infos[NUM_VOXEL_FACE_TYPES];
voxel_face_type_allocation_info_t voxel_face_type_allocation_infos[NUM_VOXEL_FACE_TYPES];

voxel_region_render_info_t voxel_region_render_info;
voxel_region_allocation_info_t voxel_region_allocation_info;

VkSampler voxel_texture_image_sampler;
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
            { "image/cube_voxel_1.png", STBI_rgb },
            { "image/cube_voxel_2.png", STBI_rgb },
            { "image/cube_voxel_3.png", STBI_rgb },
            { "image/cube_voxel_4.png", STBI_rgb },
            { "image/cube_voxel_5.png", STBI_rgb }
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

    const char* mesh_paths[] = {
        "mesh/cube_voxel.glb"
    };

    uint32_t num_vertices_array[NUM_VOXEL_FACE_TYPES];

    staging_t vertex_stagings[NUM_VOXEL_FACE_TYPES];
    staging_t index_stagings[NUM_VOXEL_FACE_TYPES];

    uint32_t num_vertex_bytes = sizeof(voxel_vertex_t);
    uint32_t num_index_bytes = sizeof(uint16_t);
    uint32_t num_instance_bytes = sizeof(voxel_face_instance_t);

    voxel_face_type_mesh_t cube_face_type_meshes[NUM_VOXEL_FACE_TYPES];

    if (load_gltf_voxel_face_type_meshes(mesh_paths[0], NUM_VOXEL_FACE_TYPES, (const char*[]) { "Front", "Back", "Top", "Bottom", "Right", "Left" }, cube_face_type_meshes) != result_success) {
        return "Failed to load mesh\n";
    }

    for (size_t i = 0; i < NUM_VOXEL_FACE_TYPES; i++) {
        // cube_voxel_render_info.faces[i].num_instances = NUM_ELEMS(face_instances);
        // voxel_face_instances.faces[i] = face_instances;

        const voxel_face_type_mesh_t* mesh = &cube_face_type_meshes[i];
        voxel_face_type_render_info_t* render_info = &voxel_face_type_render_infos[i];
        voxel_face_type_allocation_info_t* allocation_info = &voxel_face_type_allocation_infos[i];

        num_vertices_array[i] = mesh->num_vertices;
        render_info->num_indices = mesh->num_indices;

        if (begin_buffers(mesh->num_vertices, &vertex_buffer_create_info, 1, &mesh->vertices_data, &num_vertex_bytes, &vertex_stagings[i], &render_info->vertex_buffer, &allocation_info->vertex_allocation) != result_success) {
            return "Failed to begin creating vertex buffers\n"; 
        }

        if (begin_buffers(mesh->num_indices, &index_buffer_create_info, 1, &mesh->indices_data, &num_index_bytes, &index_stagings[i], &render_info->index_buffer, &allocation_info->index_allocation) != result_success) {
            return "Failed to begin creating index buffer\n";
        }

        free(mesh->vertices_data);
        free(mesh->indices_data);
    }

    voxel_region_voxel_types_t* voxel_types = memalign(64, sizeof(voxel_region_voxel_types_t));
    memset(voxel_types, voxel_type_air, sizeof(voxel_region_voxel_types_t));
    
    create_voxel_region_voxel_types(voxel_types);

    voxel_face_instance_arrays_t face_instance_arrays = { 0 };
    create_voxel_face_instance_arrays(voxel_types, &face_instance_arrays);

    free(voxel_types);
    
    voxel_region_staging_t voxel_region_staging;
    
    if (begin_voxel_region_info(&face_instance_arrays, &voxel_region_staging, &voxel_region_render_info, &voxel_region_allocation_info) != result_success) {
        return "Failed to begin creating voxel region info\n";
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

    for (size_t i = 0; i < NUM_VOXEL_FACE_TYPES; i++) {
        const voxel_face_type_render_info_t* type_render_info = &voxel_face_type_render_infos[i];
        const voxel_face_model_render_info_t* model_render_info = &voxel_region_render_info.face_model_infos[i];

        transfer_buffers(command_buffer, num_vertices_array[i], 1, &num_vertex_bytes, &vertex_stagings[i], &type_render_info->vertex_buffer);
        transfer_buffers(command_buffer, type_render_info->num_indices, 1, &num_index_bytes, &index_stagings[i], &type_render_info->index_buffer);
        if (model_render_info->num_instances != 0) {
            transfer_buffers(command_buffer, model_render_info->num_instances, 1, &num_instance_bytes, &voxel_region_staging.face_model_stagings[i], &model_render_info->instance_buffer);
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

    for (size_t i = 0; i < NUM_VOXEL_FACE_TYPES; i++) {
        end_buffers(1, &vertex_stagings[i]);
        end_buffers(1, &index_stagings[i]);
        if (voxel_region_render_info.face_model_infos[i].num_instances != 0) {
            end_buffers(1, &voxel_region_staging.face_model_stagings[i]);
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
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .minFilter = VK_FILTER_NEAREST,
        .magFilter = VK_FILTER_NEAREST,
        .anisotropyEnable = VK_FALSE,
        .maxLod = (float)image_create_infos[0].info.mipLevels
    }, NULL, &voxel_texture_image_sampler) != VK_SUCCESS) {
        return "Failed to create tetxure image sampler\n";
    }

    return NULL;
}

void term_vulkan_assets(void) {
    vkDestroySampler(device, voxel_texture_image_sampler, NULL);
    destroy_images(NUM_TEXTURE_IMAGES, texture_images, texture_image_allocations, texture_image_views);

    for (size_t i = 0; i < NUM_VOXEL_FACE_TYPES; i++) {
        const voxel_face_type_render_info_t* type_render_info = &voxel_face_type_render_infos[i];
        const voxel_face_type_allocation_info_t* type_allocation_info = &voxel_face_type_allocation_infos[i];

        const voxel_face_model_render_info_t* model_render_info = &voxel_region_render_info.face_model_infos[i];
        const voxel_face_model_allocation_info_t* model_allocation_info = &voxel_region_allocation_info.face_model_infos[i];

        vmaDestroyBuffer(allocator, type_render_info->vertex_buffer, type_allocation_info->vertex_allocation);
        vmaDestroyBuffer(allocator, type_render_info->index_buffer, type_allocation_info->index_allocation);
        if (model_render_info->num_instances != 0) {
            vmaDestroyBuffer(allocator, model_render_info->instance_buffer, model_allocation_info->instance_allocation);
        }
    }
}
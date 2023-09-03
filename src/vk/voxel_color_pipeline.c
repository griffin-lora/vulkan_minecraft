#include "voxel_color_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "asset.h"
#include "util.h"
#include "defaults.h"
#include "render.h"
#include "gfx_pipeline.h"
#include "camera.h"
#include "voxel_assets.h"
#include "voxel_dynamic_assets.h"
#include "voxel/vertex.h"
#include <vk_mem_alloc.h>
#include <stdalign.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>
#include <cglm/struct/cam.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine.h>

alignas(64) static graphics_pipeline_render_info_t pipeline_info;

typedef struct {
    mat4s view_projection;
    union {
        vec3s region_position;
        float region_position_data[4];
    };
} push_constants_t;

const char* init_voxel_color_pipeline(void) {
    if (create_descriptor_set(
        &(VkDescriptorSetLayoutCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            
            .bindingCount = 1,
            .pBindings = (VkDescriptorSetLayoutBinding[1]) {
                {
                    DEFAULT_VK_DESCRIPTOR_BINDING,
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
                }
            }
        },
        (descriptor_info_t[1]) {
            {
                .type = descriptor_info_type_image,
                .image = {
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .imageView = texture_image_views[0],
                    .sampler = voxel_region_texture_image_sampler
                }
            }
        },
        &pipeline_info.descriptor_set_layout, &pipeline_info.descriptor_pool, &pipeline_info.descriptor_set
    ) != result_success) {
        return "Failed to create descriptor set\n";
    }

    if (vkCreatePipelineLayout(device, &(VkPipelineLayoutCreateInfo) {
        DEFAULT_VK_PIPELINE_LAYOUT,
        .pSetLayouts = &pipeline_info.descriptor_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &(VkPushConstantRange) {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .size = sizeof(push_constants_t)
        }
    }, NULL, &pipeline_info.pipeline_layout) != VK_SUCCESS) {
        return "Failed to create pipeline layout\n";
    }

    //

    VkShaderModule vertex_shader_module;
    if (create_shader_module("shader/voxel_color_pipeline_vertex.spv", &vertex_shader_module) != result_success) {
        return "Failed to create vertex shader module\n";
    }

    VkShaderModule fragment_shader_module;
    if (create_shader_module("shader/voxel_color_pipeline_fragment.spv", &fragment_shader_module) != result_success) {
        return "Failed to create fragment shader module\n";
    }

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo) {
        DEFAULT_VK_GRAPHICS_PIPELINE,

        .stageCount = 2,
        .pStages = (VkPipelineShaderStageCreateInfo[2]) {
            {
                DEFAULT_VK_SHADER_STAGE,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertex_shader_module
            },
            {
                DEFAULT_VK_SHADER_STAGE,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragment_shader_module
            }
        },

        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = (VkVertexInputBindingDescription[1]) {
                {
                    .binding = 0,
                    .stride = sizeof(voxel_vertex_t),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                }
            },

            .vertexAttributeDescriptionCount = 1,
            .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[1]) {
                {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32_UINT,
                    .offset = 0
                }
            }
        },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) { DEFAULT_VK_RASTERIZATION },
        .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
            DEFAULT_VK_MULTISAMPLE,
            .rasterizationSamples = render_multisample_flags
        },
        .layout = pipeline_info.pipeline_layout,
        .renderPass = frame_render_pass
    }, NULL, &pipeline_info.pipeline) != VK_SUCCESS) {
        return "Failed to create graphics pipeline\n";
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);
    vkDestroyShaderModule(device, fragment_shader_module, NULL);

    return NULL;
}

void draw_voxel_color_pipeline(VkCommandBuffer command_buffer) {
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.pipeline_layout, 0, 1, &pipeline_info.descriptor_set, 0, NULL);

    push_constants_t push_constants = { 0 };

    vkCmdPushConstants(command_buffer, pipeline_info.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, offsetof(push_constants_t, view_projection), sizeof(push_constants.view_projection), &camera_view_projection);

    pthread_mutex_lock(&voxel_dynamic_assets_front_index.mutex);
    const voxel_region_render_info_t* render_infos = voxel_region_render_info_arrays[voxel_dynamic_assets_front_index.index];

    for (size_t i = 0; i < NUM_VOXEL_REGIONS; i++) {
        const voxel_region_render_info_t* render_info = &render_infos[i];

        if (render_info->num_vertices == 0) {
            continue;
        }


        vkCmdPushConstants(command_buffer, pipeline_info.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, offsetof(push_constants_t, region_position), sizeof(push_constants.region_position), &render_info->position);

        vkCmdBindVertexBuffers(command_buffer, 0, 1, &render_info->vertex_buffer, (VkDeviceSize[1]) { 0 });
        vkCmdDraw(command_buffer, render_info->num_vertices, 1, 0, 0);
    }

    pthread_mutex_unlock(&voxel_dynamic_assets_front_index.mutex);
}

void term_voxel_color_pipeline(void) {
    destroy_graphics_pipeline(&pipeline_info);
}
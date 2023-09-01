#include "text_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "defaults.h"
#include "asset.h"
#include "gfx_pipeline.h"
#include "text_assets.h"
#include "render.h"
#include <stdalign.h>
#include <string.h>
#include <stdio.h>

alignas(64) static graphics_pipeline_render_info_t pipeline_info;

typedef struct {
    vec2s size_reciprocal;
    vec2s model_position;
} push_constants_t;

const char* init_text_pipeline(void) {
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
                    .imageView = texture_image_views[1],
                    .sampler = text_texture_image_sampler
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
    if (create_shader_module("shader/text_pipeline_vertex.spv", &vertex_shader_module) != result_success) {
        return "Failed to create vertex shader module\n";
    }

    VkShaderModule fragment_shader_module;
    if (create_shader_module("shader/text_pipeline_fragment.spv", &fragment_shader_module) != result_success) {
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
        
            .vertexBindingDescriptionCount = 2,
            .pVertexBindingDescriptions = (VkVertexInputBindingDescription[2]) {
                {
                    .binding = 0,
                    .stride = sizeof(text_glyph_instance_t),
                    .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
                },
                {
                    .binding = 1,
                    .stride = sizeof(text_glyph_vertex_t),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                }
            },

            .vertexAttributeDescriptionCount = 4,
            .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[4]) {
                {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(text_glyph_instance_t, position)
                },
                {
                    .binding = 0,
                    .location = 1,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(text_glyph_instance_t, tex_coord)
                },
                {
                    .binding = 1,
                    .location = 2,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(text_glyph_vertex_t, position)
                },
                {
                    .binding = 1,
                    .location = 3,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(text_glyph_vertex_t, tex_coord)
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

void draw_text_pipeline(VkCommandBuffer command_buffer) {
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.pipeline_layout, 0, 1, &pipeline_info.descriptor_set, 0, NULL);

    push_constants_t push_constants = { 0 };
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    // Because [-1, 1] coordinates
    width /= 2;
    height /= 2;

    push_constants.size_reciprocal = (vec2s) {{ 1.0f/(float)width, 1.0f/(float)height }};
    vkCmdPushConstants(command_buffer, pipeline_info.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, offsetof(push_constants_t, size_reciprocal), sizeof(push_constants.size_reciprocal), &push_constants.size_reciprocal);
    
    for (size_t i = 0; i < NUM_TEXT_MODELS; i++) {
        const text_model_render_info_t* model_render_info = &text_model_render_infos[i];

        if (model_render_info->num_instances == 0) {
            continue;
        }

        vkCmdPushConstants(command_buffer, pipeline_info.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, offsetof(push_constants_t, model_position), sizeof(push_constants.model_position), &model_render_info->model_position);
        
        bind_vertex_buffers(command_buffer, 2, (VkBuffer[2]) {
            model_render_info->instance_buffer,
            text_glyph_render_info.vertex_buffer
        });

        vkCmdBindIndexBuffer(command_buffer, text_glyph_render_info.index_buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(command_buffer, NUM_TEXT_GLYPH_INDICES, model_render_info->num_instances, 0, 0, 0);
    }
}

void term_text_pipeline(void) {
    destroy_graphics_pipeline(&pipeline_info);
}
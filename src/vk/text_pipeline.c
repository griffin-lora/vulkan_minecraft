#include "text_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "defaults.h"
#include "asset.h"
#include "gfx_pipeline.h"
#include "render.h"
#include <stdalign.h>
#include <string.h>

typedef struct {
    vec2s model_position;
    VkBuffer instance_buffer;
    uint32_t num_instances;
} text_model_render_info_t;

typedef struct {
    VmaAllocation instance_allocation;
} text_model_allocation_info_t;

alignas(64)
static graphics_pipeline_render_info_t pipeline_info;
text_glyph_render_info_t text_glyph_render_info = { 0 };
text_glyph_allocation_info_t text_glyph_allocation_info = { 0 };

#define NUM_TEXT_MODELS 1

alignas(64) static text_model_render_info_t text_model_render_infos[NUM_TEXT_MODELS] = { 0 };
alignas(64) static text_model_allocation_info_t text_model_allocation_infos[NUM_TEXT_MODELS] = { 0 };
alignas(64) static VkBuffer text_model_staging_buffers[NUM_TEXT_MODELS] = { 0 };
alignas(64) static VmaAllocation text_model_staging_allocations[NUM_TEXT_MODELS] = { 0 };

alignas(64)
static size_t num_text_model_staging_updates = 0;
static size_t text_model_staging_updates[NUM_TEXT_MODELS] = { 0 };

result_t init_text_model(size_t index, vec2s model_position, uint32_t num_glyphs) {
    if (index >= NUM_TEXT_MODELS) {
        return result_failure;
    }

    text_model_render_info_t* render_info = &text_model_render_infos[index];
    text_model_allocation_info_t* allocation_info = &text_model_allocation_infos[index];
    VkBuffer* staging_buffer = &text_model_staging_buffers[index];
    VmaAllocation* staging_allocation = &text_model_staging_allocations[index];

    render_info->model_position = model_position;

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_STAGING_BUFFER,
        .size = num_glyphs*sizeof(text_glyph_instance_t)
    }, &staging_allocation_create_info, staging_buffer, staging_allocation, NULL) != VK_SUCCESS) {
        return result_failure;
    }

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_VERTEX_BUFFER,
        .size = num_glyphs*sizeof(text_glyph_instance_t)
    }, &device_allocation_create_info, &render_info->instance_buffer, &allocation_info->instance_allocation, NULL) != VK_SUCCESS) {
        return result_failure;
    }

    return result_success;
}

result_t set_text_model_message(size_t index, const char* message) {
    if (index >= NUM_TEXT_MODELS || num_text_model_staging_updates >= NUM_TEXT_MODELS) {
        return result_failure;
    }

    uint32_t num_glyphs = (uint32_t)strlen(message);

    text_glyph_instance_t instances[num_glyphs];
    for (uint32_t i = 0; i < num_glyphs; i++) {
        instances[i] = (text_glyph_instance_t) {
            .position = {{ (float)i * TEXT_GLYPH_SIZE, 0.0f }},
            .tex_coord = {{ 0.0f, 0.0f }},
        };
    }

    if (write_to_buffer(text_model_staging_allocations[index], sizeof(instances), instances) != result_success) {
        return result_failure;
    }

    text_model_staging_updates[num_text_model_staging_updates++] = index;
    
    return result_success;
}

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
                    .imageView = texture_image_views[0],
                    .sampler = voxel_texture_image_sampler
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
            .size = sizeof(vec2s)
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

            .vertexAttributeDescriptionCount = 3,
            .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[3]) {
                {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(text_glyph_instance_t, tex_coord)
                },
                {
                    .binding = 1,
                    .location = 1,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(text_glyph_vertex_t, position)
                },
                {
                    .binding = 1,
                    .location = 2,
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
    
    for (size_t i = 0; i < NUM_TEXT_MODELS; i++) {
        const text_model_render_info_t* model_render_info = &text_model_render_infos[i];

        if (model_render_info->num_instances == 0) {
            continue;
        }

        vkCmdPushConstants(command_buffer, pipeline_info.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vec2s), &model_render_info->model_position);
        
        bind_vertex_buffers(command_buffer, 2, (VkBuffer[2]) {
            model_render_info->instance_buffer,
            text_glyph_render_info.vertex_buffer
        });

        vkCmdBindIndexBuffer(command_buffer, text_glyph_render_info.index_buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(command_buffer, text_glyph_render_info.num_indices, model_render_info->num_instances, 0, 0, 0);
    }
}

void term_text_pipeline(void) {
    for (size_t i = 0; i < NUM_TEXT_MODELS; i++) {
        if (text_model_render_infos[i].instance_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, text_model_render_infos[i].instance_buffer, text_model_allocation_infos[i].instance_allocation);
            vmaDestroyBuffer(allocator, text_model_staging_buffers[i], text_model_staging_allocations[i]);
        }
    }

    vkDestroyPipeline(device, pipeline_info.pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_info.pipeline_layout, NULL);
    vkDestroyDescriptorPool(device, pipeline_info.descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device, pipeline_info.descriptor_set_layout, NULL);
}
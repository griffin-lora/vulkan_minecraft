#include "shadow_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "asset.h"
#include "util.h"
#include "mesh.h"
#include "defaults.h"
#include <vk_mem_alloc.h>
#include <stdalign.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>
#include <cglm/struct/cam.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine.h>

alignas(64)
static VkRenderPass render_pass;
static VkDescriptorSetLayout descriptor_set_layout;
static VkDescriptorPool descriptor_pool;
static VkDescriptorSet descriptor_set;
static VkPipelineLayout pipeline_layout;
static VkPipeline pipeline;

#define SHADOW_IMAGE_SIZE 4096

static VkImage shadow_image;
static VmaAllocation shadow_image_allocation;
VkImageView shadow_image_view;

const char* init_shadow_pipeline(void) {
    if (vmaCreateImage(allocator, &(VkImageCreateInfo) {
        DEFAULT_VK_IMAGE,
        .extent.width = SHADOW_IMAGE_SIZE,
        .extent.height = SHADOW_IMAGE_SIZE,
        .format = depth_image_format,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    }, &device_allocation_create_info, &shadow_image, &shadow_image_allocation, NULL) != VK_SUCCESS) {
        return "Failed to create shadow image\n";
    }

    if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
        DEFAULT_VK_IMAGE_VIEW,
        .image = shadow_image,
        .format = depth_image_format,
        .subresourceRange.aspectMask = (depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT
    }, NULL, &shadow_image_view) != VK_SUCCESS) {
        return "Failed to create shadow image view\n";
    }

    if (vkCreateRenderPass(device, &(VkRenderPassCreateInfo) {
        DEFAULT_VK_RENDER_PASS,

        .attachmentCount = 1,
        .pAttachments = &(VkAttachmentDescription) {
            DEFAULT_VK_ATTACHMENT,
            .format = depth_image_format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },

        .pSubpasses = &(VkSubpassDescription) {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .pDepthStencilAttachment = &(VkAttachmentReference) {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            }
        }
    }, NULL, &render_pass) != VK_SUCCESS) {
        return "Failed to create render pass\n";
    }

    if (create_descriptor_set(
        &(VkDescriptorSetLayoutCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &(VkDescriptorSetLayoutBinding) {
                DEFAULT_VK_DESCRIPTOR_BINDING,
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            }
        },
        
        &(descriptor_info_t) {
            .type = descriptor_info_type_buffer,
            .buffer = {
                .buffer = shadow_view_projection_buffer,
                .offset = 0,
                .range = sizeof(shadow_view_projection)
            }
        },

        &descriptor_set_layout, &descriptor_pool, &descriptor_set
    ) != result_success) {
        return "Failed to create descriptor set\n";
    }

    if (vkCreatePipelineLayout(device, &(VkPipelineLayoutCreateInfo) {
        DEFAULT_VK_PIPELINE_LAYOUT,
        .pSetLayouts = &descriptor_set_layout
    }, NULL, &pipeline_layout) != VK_SUCCESS) {
        return "Failed to create pipeline layout\n";
    }

    VkShaderModule vertex_shader_module;
    if (create_shader_module("shader/shadow_pipeline_vertex.spv", &vertex_shader_module) != result_success) {
        return "Failed to create vertex shader module\n";
    }

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo) {
        DEFAULT_VK_GRAPHICS_PIPELINE,

        .stageCount = 1,
        .pStages = &(VkPipelineShaderStageCreateInfo) {
            DEFAULT_VK_SHADER_STAGE,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module
        },

        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,

            .vertexBindingDescriptionCount = 2,
            .pVertexBindingDescriptions = (VkVertexInputBindingDescription[2]) {
                {
                    .binding = 0,
                    .stride = sizeof(mat4s),
                    .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
                },
                {
                    .binding = 1,
                    .stride = num_vertex_bytes_array[GENERAL_PIPELINE_VERTEX_ARRAY_INDEX],
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                }
            },

            .vertexAttributeDescriptionCount = 5,
            .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[5]) {
                {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = 0*sizeof(vec4s)
                },
                {
                    .binding = 0,
                    .location = 1,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = 1*sizeof(vec4s)
                },
                {
                    .binding = 0,
                    .location = 2,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = 2*sizeof(vec4s)
                },
                {
                    .binding = 0,
                    .location = 3,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = 3*sizeof(vec4s)
                },
                //
                {
                    .binding = 1,
                    .location = 4,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(general_pipeline_vertex_t, position)
                }
            }
        },

        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
            DEFAULT_VK_RASTERIZATION,
            .depthBiasEnable = VK_TRUE,
            .depthBiasConstantFactor = 4.0f,
            .depthBiasSlopeFactor = 1.5f
        },
        .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) { DEFAULT_VK_MULTISAMPLE },
        .pColorBlendState = NULL,
        .layout = pipeline_layout,
        .renderPass = render_pass
    }, NULL, &pipeline) != VK_SUCCESS) {
        return "Failed to create graphics pipeline\n";
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);

    return NULL;
}

const char* draw_shadow_pipeline(void) {
    VkFence render_fence;
    if (vkCreateFence(device, &(VkFenceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    }, NULL, &render_fence) != VK_SUCCESS) {
        return "Failed to create render fence\n";
    }

    VkCommandBuffer command_buffer;
    if (vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = command_pool
    }, &command_buffer) != VK_SUCCESS) {
        return "Failed to create command buffer\n";
    }

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }) != VK_SUCCESS) {
        return "Failed to write to command buffer\n";
    }

    VkFramebuffer framebuffer;
    if (vkCreateFramebuffer(device, &(VkFramebufferCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .pAttachments = &shadow_image_view,
        .width = SHADOW_IMAGE_SIZE,
        .height = SHADOW_IMAGE_SIZE,
        .layers = 1
    }, NULL, &framebuffer) != VK_SUCCESS) {
        return "Failed to create shadow image framebuffer\n";
    }

    begin_pipeline(
        command_buffer,
        framebuffer, (VkExtent2D) { .width = SHADOW_IMAGE_SIZE, .height = SHADOW_IMAGE_SIZE },
        1, &(VkClearValue) { .depthStencil = { .depth = 1.0f, .stencil = 0 } },
        render_pass, descriptor_set, pipeline_layout, pipeline
    );

    for (size_t i = 0; i < NUM_MODELS; i++) {
        bind_vertex_buffers(command_buffer, 2, (VkBuffer[2]) {
            instance_buffers[i],
            vertex_buffer_arrays[i][GENERAL_PIPELINE_VERTEX_ARRAY_INDEX]
        });
        vkCmdBindIndexBuffer(command_buffer, index_buffers[i], 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(command_buffer, num_indices_array[i], num_instances_array[i], 0, 0, 0);
    }

    end_pipeline(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return "Failed to write to transfer command buffer\n";
    }

    vkQueueSubmit(graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer
    }, render_fence);
    vkWaitForFences(device, 1, &render_fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device, render_fence, NULL);

    vkDestroyFramebuffer(device, framebuffer, NULL);

    return NULL;
}

void term_shadow_pipeline(void) {
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);


    vkDestroyImageView(device, shadow_image_view, NULL);
    vmaDestroyImage(allocator, shadow_image, shadow_image_allocation);
}
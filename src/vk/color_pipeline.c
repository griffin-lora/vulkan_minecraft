#include "color_pipeline.h"
#include "core.h"
#include "gfx_core.h"
#include "asset.h"
#include "util.h"
#include "mesh.h"
#include "defaults.h"
#include "voxel/face_instance.h"
#include <vk_mem_alloc.h>
#include <stdalign.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>
#include <cglm/struct/cam.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine.h>

alignas(64)
VkRenderPass color_pipeline_render_pass;
static VkDescriptorSetLayout descriptor_set_layout;
static VkDescriptorPool descriptor_pool;
static VkDescriptorSet descriptor_set;
static VkPipelineLayout pipeline_layout;
static VkPipeline pipeline;

static VkCommandBuffer color_command_buffers[NUM_FRAMES_IN_FLIGHT];

static VkImage color_image;
static VmaAllocation color_image_allocation;
VkImageView color_image_view;

static VkImage depth_image;
static VmaAllocation depth_image_allocation;
VkImageView depth_image_view;

color_pipeline_push_constants_t color_pipeline_push_constants;

result_t init_color_pipeline_swapchain_dependents(void) {
    if (vmaCreateImage(allocator, &(VkImageCreateInfo) {
        DEFAULT_VK_IMAGE,
        .extent.width = swap_image_extent.width,
        .extent.height = swap_image_extent.height,
        .format = surface_format.format,
        .samples = render_multisample_flags,
        .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    }, &device_allocation_create_info, &color_image, &color_image_allocation, NULL) != VK_SUCCESS) {
        return result_failure;
    }

    if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
        DEFAULT_VK_IMAGE_VIEW,
        .image = color_image,
        .format = surface_format.format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
    }, NULL, &color_image_view) != VK_SUCCESS) {
        return result_failure;
    }

    if (vmaCreateImage(allocator, &(VkImageCreateInfo) {
        DEFAULT_VK_IMAGE,
        .extent.width = swap_image_extent.width,
        .extent.height = swap_image_extent.height,
        .format = depth_image_format,
        .samples = render_multisample_flags,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    }, &device_allocation_create_info, &depth_image, &depth_image_allocation, NULL) != VK_SUCCESS) {
        return result_failure;
    }
    
    if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
        DEFAULT_VK_IMAGE_VIEW,
        .image = depth_image,
        .format = depth_image_format,
        .subresourceRange.aspectMask = (depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT
    }, NULL, &depth_image_view) != VK_SUCCESS) {
        return result_failure;
    }

    return result_success;
}

void term_color_pipeline_swapchain_dependents(void) {
    vkDestroyImageView(device, color_image_view, NULL);
    vmaDestroyImage(allocator, color_image, color_image_allocation);
    vkDestroyImageView(device, depth_image_view, NULL);
    vmaDestroyImage(allocator, depth_image, depth_image_allocation);
}

const char* init_color_pipeline(void) {
    if (init_color_pipeline_swapchain_dependents() != result_success) {
        return "Failed to create color pipeline images\n";
    }

    if (vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = command_pool,
        .commandBufferCount = NUM_FRAMES_IN_FLIGHT
    }, color_command_buffers) != VK_SUCCESS) {
        return "Failed to allocate command buffers\n";
    }

    if (vkCreateRenderPass(device, &(VkRenderPassCreateInfo) {
        DEFAULT_VK_RENDER_PASS,

        .attachmentCount = 3,
        .pAttachments = (VkAttachmentDescription[3]) {
            {
                DEFAULT_VK_ATTACHMENT,
                .format = surface_format.format,
                .samples = render_multisample_flags,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },
            {
                DEFAULT_VK_ATTACHMENT,
                .format = depth_image_format,
                .samples = render_multisample_flags,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            },
            {
                DEFAULT_VK_ATTACHMENT,
                .format = surface_format.format,
                .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            }
        },

        .pSubpasses = &(VkSubpassDescription) {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,

            .colorAttachmentCount = 1,
            .pColorAttachments = &(VkAttachmentReference) {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },

            .pDepthStencilAttachment = &(VkAttachmentReference) {
                .attachment = 1,
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            },

            .pResolveAttachments = &(VkAttachmentReference) {
                .attachment = 2,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            }
        },

        .dependencyCount = 1,
        .pDependencies = &(VkSubpassDependency) {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        }
    }, NULL, &color_pipeline_render_pass) != VK_SUCCESS) {
        return "Failed to create render pass\n";
    }

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
        &descriptor_set_layout, &descriptor_pool, &descriptor_set
    ) != result_success) {
        return "Failed to create descriptor set\n";
    }

    if (vkCreatePipelineLayout(device, &(VkPipelineLayoutCreateInfo) {
        DEFAULT_VK_PIPELINE_LAYOUT,
        .pSetLayouts = &descriptor_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &(VkPushConstantRange) {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .size = sizeof(color_pipeline_push_constants)
        }
    }, NULL, &pipeline_layout) != VK_SUCCESS) {
        return "Failed to create pipeline layout\n";
    }

    //

    VkShaderModule vertex_shader_module;
    if (create_shader_module("shader/color_pipeline_vertex.spv", &vertex_shader_module) != result_success) {
        return "Failed to create vertex shader module\n";
    }

    VkShaderModule fragment_shader_module;
    if (create_shader_module("shader/color_pipeline_fragment.spv", &fragment_shader_module) != result_success) {
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
                    .stride = sizeof(voxel_face_instance_t),
                    .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
                },
                {
                    .binding = 1,
                    .stride = num_vertex_bytes_array[GENERAL_PIPELINE_VERTEX_ARRAY_INDEX],
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                }
            },

            .vertexAttributeDescriptionCount = 3,
            .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[3]) {
                {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32_UINT,
                    .offset = offsetof(voxel_face_instance_t, info)
                },
                {
                    .binding = 1,
                    .location = 1,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(voxel_vertex_t, position)
                },
                {
                    .binding = 1,
                    .location = 2,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(voxel_vertex_t, tex_coord)
                }
            }
        },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) { DEFAULT_VK_RASTERIZATION },
        .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
            DEFAULT_VK_MULTISAMPLE,
            .rasterizationSamples = render_multisample_flags
        },
        .layout = pipeline_layout,
        .renderPass = color_pipeline_render_pass
    }, NULL, &pipeline) != VK_SUCCESS) {
        return "Failed to create graphics pipeline\n";
    }

    vkDestroyShaderModule(device, vertex_shader_module, NULL);
    vkDestroyShaderModule(device, fragment_shader_module, NULL);

    return NULL;
}

const char* draw_color_pipeline(size_t frame_index, size_t image_index, VkCommandBuffer* out_command_buffer) {
    VkCommandBuffer command_buffer = color_command_buffers[frame_index];
    *out_command_buffer = command_buffer;

    vkResetCommandBuffer(command_buffer, 0);
    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    }) != VK_SUCCESS) {
        return "Failed to begin writing to command buffer\n";
    }

    begin_pipeline(
        command_buffer,
        swapchain_framebuffers[image_index], swap_image_extent,
        2, (VkClearValue[2]) {
            { .color = { .float32 = { 0.62f, 0.78f, 1.0f, 1.0f } } },
            { .depthStencil = { .depth = 1.0f, .stencil = 0 } },
        },
        color_pipeline_render_pass, descriptor_set, pipeline_layout, pipeline
    );

    vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(color_pipeline_push_constants), &color_pipeline_push_constants);

    for (size_t i = 0; i < NUM_VOXEL_FACE_TYPES; i++) {
        const voxel_face_type_render_info_t* type_render_info = &voxel_face_type_render_infos[i];
        const voxel_face_model_render_info_t* model_render_info = &voxel_region_render_info.face_model_infos[i];

        if (model_render_info->num_instances == 0) {
            continue;
        }

        bind_vertex_buffers(command_buffer, 2, (VkBuffer[2]) {
            model_render_info->instance_buffer,
            type_render_info->vertex_buffer
        });
        vkCmdBindIndexBuffer(command_buffer, type_render_info->index_buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(command_buffer, type_render_info->num_indices, model_render_info->num_instances, 0, 0, 0);
    }

    end_pipeline(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return "Failed to end command buffer\n";
    }

    return NULL;
}

void term_color_pipeline(void) {
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, color_pipeline_render_pass, NULL);
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);

    term_color_pipeline_swapchain_dependents();
}
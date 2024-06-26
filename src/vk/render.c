#include "render.h"
#include "core.h"
#include "gfx_pipeline.h"
#include "result.h"
#include "defaults.h"
#include "text_assets.h"
#include "timing.h"
#include <string.h>
#include <stdio.h>

VkRenderPass frame_render_pass;
static VkCommandPool frame_command_pool;
static VkCommandBuffer frame_command_buffers[NUM_FRAMES_IN_FLIGHT];

static VkImage frame_color_image;
static VmaAllocation frame_color_image_allocation;
VkImageView frame_color_image_view;

static VkImage frame_depth_image;
static VmaAllocation frame_depth_image_allocation;
VkImageView frame_depth_image_view;

static uint32_t frame_index = 0;

result_t init_frame_swapchain_dependents(void) {
    if (vmaCreateImage(allocator, &(VkImageCreateInfo) {
        DEFAULT_VK_IMAGE,
        .extent.width = swap_image_extent.width,
        .extent.height = swap_image_extent.height,
        .format = surface_format.format,
        .samples = render_multisample_flags,
        .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    }, &device_allocation_create_info, &frame_color_image, &frame_color_image_allocation, NULL) != VK_SUCCESS) {
        return result_image_create_failure;
    }

    if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
        DEFAULT_VK_IMAGE_VIEW,
        .image = frame_color_image,
        .format = surface_format.format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
    }, NULL, &frame_color_image_view) != VK_SUCCESS) {
        return result_image_view_create_failure;
    }

    if (vmaCreateImage(allocator, &(VkImageCreateInfo) {
        DEFAULT_VK_IMAGE,
        .extent.width = swap_image_extent.width,
        .extent.height = swap_image_extent.height,
        .format = depth_image_format,
        .samples = render_multisample_flags,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    }, &device_allocation_create_info, &frame_depth_image, &frame_depth_image_allocation, NULL) != VK_SUCCESS) {
        return result_image_create_failure;
    }
    
    if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
        DEFAULT_VK_IMAGE_VIEW,
        .image = frame_depth_image,
        .format = depth_image_format,
        .subresourceRange.aspectMask = (depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT
    }, NULL, &frame_depth_image_view) != VK_SUCCESS) {
        return result_image_view_create_failure;
    }

    return result_success;
}

void term_frame_swapchain_dependents(void) {
    vkDestroyImageView(device, frame_color_image_view, NULL);
    vmaDestroyImage(allocator, frame_color_image, frame_color_image_allocation);
    vkDestroyImageView(device, frame_depth_image_view, NULL);
    vmaDestroyImage(allocator, frame_depth_image, frame_depth_image_allocation);
}

result_t init_frame_rendering(void) {
    result_t result;

    if ((result = init_frame_swapchain_dependents()) != result_success) {
        return result;
    }
    
    if (vkCreateCommandPool(device, &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_indices.graphics
    }, NULL, &frame_command_pool) != VK_SUCCESS) {
        return result_command_pool_create_failure;
    }

    if (vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = frame_command_pool,
        .commandBufferCount = NUM_FRAMES_IN_FLIGHT
    }, frame_command_buffers) != VK_SUCCESS) {
        return result_command_buffers_allocate_failure;
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
    }, NULL, &frame_render_pass) != VK_SUCCESS) {
        return result_render_pass_create_failure;
    }

    return result_success;
}

result_t draw_frame(float) {
    microseconds_t start_time = get_current_microseconds();

    VkSemaphore image_available_semaphore = image_available_semaphores[frame_index];
    VkSemaphore render_finished_semaphore = render_finished_semaphores[frame_index];
    VkFence in_flight_fence = in_flight_fences[frame_index];

    vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);

    pthread_mutex_lock(&command_buffer_finished_mutex);
    command_buffer_finished_statuses[frame_index] = true;
    pthread_cond_signal(&command_buffer_finished_condition);
    pthread_mutex_unlock(&command_buffer_finished_mutex);

    in_flight_time = get_current_microseconds() - start_time;
    start_time = get_current_microseconds();

    uint32_t image_index;
    {
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            reinit_swapchain();
            return result_success;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            return result_swapchain_image_acquire_failure;
        }
    }

    vkResetFences(device, 1, &in_flight_fence);

    VkCommandBuffer command_buffer = frame_command_buffers[frame_index];

    vkResetCommandBuffer(command_buffer, 0);

    begin_frame_time = get_current_microseconds() - start_time;
    start_time = get_current_microseconds();

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    update_text_assets(command_buffer);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)swap_image_extent.width,
        .height = (float)swap_image_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = swap_image_extent
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBeginRenderPass(command_buffer, &(VkRenderPassBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = frame_render_pass,
        .framebuffer = swapchain_framebuffers[image_index],
        .renderArea.offset = { 0, 0 },
        .renderArea.extent = swap_image_extent,
        .clearValueCount = 2,
        .pClearValues = (VkClearValue[2]) {
            { .color = { .float32 = { 0.62f, 0.78f, 1.0f, 1.0f } } },
            { .depthStencil = { .depth = 1.0f, .stencil = 0 } },
        }
    }, VK_SUBPASS_CONTENTS_INLINE);
    
    draw_graphics_pipelines(command_buffer);
    
    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return result_command_buffer_end_failure;
    }

    write_command_buffer_time = get_current_microseconds() - start_time;
    start_time = get_current_microseconds();

    VkPipelineStageFlags wait_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    
    pthread_mutex_lock(&queue_submit_mutex);
    if (vkQueueSubmit(graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_available_semaphore,
        .pWaitDstStageMask = &wait_stage_flags,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &render_finished_semaphore
    }, in_flight_fence) != VK_SUCCESS) {
        return result_queue_submit_failure;
    }
    pthread_mutex_unlock(&queue_submit_mutex);

    {
        VkResult result = vkQueuePresentKHR(presentation_queue, &(VkPresentInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &image_index
        });
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
            framebuffer_resized = false;
            vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
            reinit_swapchain();
        } else if (result != VK_SUCCESS) {
            return result_swapchain_image_present_failure;
        }
    }

    frame_index += 1;
    frame_index %= NUM_FRAMES_IN_FLIGHT;

    end_frame_time = get_current_microseconds() - start_time;

    return result_success;
}

void term_frame_rendering(void) {
    vkDestroyRenderPass(device, frame_render_pass, NULL);

    vkDestroyCommandPool(device, frame_command_pool, NULL);

    term_frame_swapchain_dependents();
}
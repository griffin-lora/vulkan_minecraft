#include "dynamic_asset_transfer.h"
#include "vk/core.h"
#include "vk/defaults.h"
#include <stdalign.h>

VkCommandBuffer dynamic_asset_transfer_command_buffer;
static VkFence dynamic_asset_transfer_fence;

const char* init_dynamic_asset_transfer(void) {
    if (vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = command_pool,
        .commandBufferCount = 1
    }, &dynamic_asset_transfer_command_buffer) != VK_SUCCESS) {
        return "Failed to allocate dynamic asset transfer command buffers\n";
    }

    if (vkCreateFence(device, &(VkFenceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    }, NULL, &dynamic_asset_transfer_fence) != VK_SUCCESS) {
        return "Failed to create dynamic asset transfer fence\n";
    }

    return NULL;
}

result_t begin_dynamic_asset_transfer(void) {
    vkResetCommandBuffer(dynamic_asset_transfer_command_buffer, 0);

    if (vkBeginCommandBuffer(dynamic_asset_transfer_command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    }) != VK_SUCCESS) {
        return result_failure;
    }

    return result_success;
}

result_t end_dynamic_asset_transfer(void) {
    if (vkEndCommandBuffer(dynamic_asset_transfer_command_buffer) != VK_SUCCESS) {
        return result_failure;
    }

    // TODO: Use transfer queue
    if (vkQueueSubmit(graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &dynamic_asset_transfer_command_buffer
    }, dynamic_asset_transfer_fence) != VK_SUCCESS) {
        return result_failure;
    }

    if (vkWaitForFences(device, 1, &dynamic_asset_transfer_fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        return result_failure;
    }

    vkResetFences(device, 1, &dynamic_asset_transfer_fence);
    
    return result_success;
}

void term_dynamic_asset_transfer(void) {
    vkDestroyFence(device, dynamic_asset_transfer_fence, NULL);
}
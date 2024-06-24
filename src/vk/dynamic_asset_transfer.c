#include "dynamic_asset_transfer.h"
#include "result.h"
#include "vk/core.h"
#include "vk/defaults.h"
#include <stdalign.h>

VkCommandPool dynamic_asset_transfer_command_pool;
VkCommandBuffer dynamic_asset_transfer_command_buffer;
static VkFence dynamic_asset_transfer_fence;

result_t init_dynamic_asset_transfer(void) {
    if (vkCreateCommandPool(device, &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_indices.graphics
    }, NULL, &dynamic_asset_transfer_command_pool) != VK_SUCCESS) {
        return result_command_pool_create_failure;
    }

    if (vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = dynamic_asset_transfer_command_pool
    }, &dynamic_asset_transfer_command_buffer) != VK_SUCCESS) {
        return result_command_buffers_allocate_failure;
    }

    if (vkCreateFence(device, &(VkFenceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    }, NULL, &dynamic_asset_transfer_fence) != VK_SUCCESS) {
        return result_synchronization_primitive_create_failure;
    }

    return result_success;
}

result_t begin_dynamic_asset_transfer(void) {
    vkResetCommandBuffer(dynamic_asset_transfer_command_buffer, 0);

    if (vkBeginCommandBuffer(dynamic_asset_transfer_command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    return result_success;
}

result_t end_dynamic_asset_transfer(void) {
    if (vkEndCommandBuffer(dynamic_asset_transfer_command_buffer) != VK_SUCCESS) {
        return result_failure;
    }

    pthread_mutex_lock(&queue_submit_mutex);
    if (vkQueueSubmit(graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &dynamic_asset_transfer_command_buffer
    }, dynamic_asset_transfer_fence) != VK_SUCCESS) {
        return result_failure;
    }
    pthread_mutex_unlock(&queue_submit_mutex);

    if (vkWaitForFences(device, 1, &dynamic_asset_transfer_fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        return result_failure;
    }

    vkResetFences(device, 1, &dynamic_asset_transfer_fence);
    
    return result_success;
}

void term_dynamic_asset_transfer(void) {
    vkDestroyFence(device, dynamic_asset_transfer_fence, NULL);
    
    vkDestroyCommandPool(device, dynamic_asset_transfer_command_pool, NULL);
}
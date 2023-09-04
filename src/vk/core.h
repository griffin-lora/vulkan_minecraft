#pragma once
#include "vk.h"
#include <stdbool.h>
#include <vk_mem_alloc.h>
#include <pthread.h>

#define NUM_FRAMES_IN_FLIGHT 2

typedef union {
    uint32_t data[2];
    struct {
        uint32_t graphics;
        uint32_t presentation;
    };
} queue_family_indices_t;

extern GLFWwindow* window;
extern VkDevice device;
extern VkPhysicalDevice physical_device;
extern VmaAllocator allocator;
extern queue_family_indices_t queue_family_indices;
extern VkSurfaceFormatKHR surface_format;
extern VkPresentModeKHR present_mode;
extern VkSemaphore image_available_semaphores[];
extern VkSemaphore render_finished_semaphores[];
extern VkFence in_flight_fences[];
extern pthread_mutex_t command_buffer_finished_conditions_mutexes[];
extern pthread_cond_t command_buffer_finished_conditions[];
extern uint32_t num_swapchain_images;
extern VkImage* swapchain_images;
extern VkImageView* swapchain_image_views;
extern VkFramebuffer* swapchain_framebuffers;
extern VkSwapchainKHR swapchain;
extern VkInstance instance;
extern VkSurfaceKHR surface;
extern VkExtent2D swap_image_extent;
extern pthread_mutex_t queue_submit_mutex;
extern VkQueue graphics_queue;
extern VkQueue presentation_queue;
extern bool framebuffer_resized;

extern VkSampleCountFlagBits render_multisample_flags;

extern VkFormat depth_image_format;

void reinit_swapchain(void);

const char* init_core(void);
void term_all(void);
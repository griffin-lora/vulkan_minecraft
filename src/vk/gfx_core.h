#pragma once
#include "result.h"
#include <vk_mem_alloc.h>
#include <stdbool.h>

// Graphics pipeline exclusive functions
result_t create_shader_module(const char* path, VkShaderModule* shader_module);
result_t write_to_buffer(VmaAllocation buffer_allocation, size_t num_bytes, const void* data);
result_t writes_to_buffer(VmaAllocation buffer_allocation, size_t num_write_bytes, size_t num_writes, const void* const data_array[]);

typedef struct {
    VkBuffer buffer;
    VmaAllocation allocation;
} staging_t;

typedef struct {
    void** pixel_arrays;
    VkDeviceSize num_pixel_bytes;
    VkImageCreateInfo info;
} image_create_info_t;

result_t begin_images(size_t num_images, const image_create_info_t infos[], staging_t stagings[], VkImage images[], VmaAllocation allocations[]);
void transfer_images(VkCommandBuffer command_buffer, size_t num_images, const image_create_info_t infos[], const staging_t stagings[], const VkImage images[]);
void end_images(size_t num_images, const staging_t stagings[]);

result_t begin_buffer(
    const VkBufferCreateInfo* base_device_buffer_create_info,
    VkDeviceSize num_elements, uint32_t num_element_bytes, const void* array,
    staging_t* staging, VkBuffer* buffer, VmaAllocation* allocation
);

typedef struct {
    enum {
        descriptor_info_type_buffer,
        descriptor_info_type_image
    } type;
    union {
        VkDescriptorBufferInfo buffer;
        VkDescriptorImageInfo image;
    };
} descriptor_info_t;

result_t create_descriptor_set(const VkDescriptorSetLayoutCreateInfo* info, descriptor_info_t infos[], VkDescriptorSetLayout* descriptor_set_layout, VkDescriptorPool* descriptor_pool, VkDescriptorSet* descriptor_set);

// Used by core as well
void destroy_images(size_t num_images, const VkImage images[], const VmaAllocation image_allocations[], const VkImageView image_views[]);

void begin_pipeline(
    VkCommandBuffer command_buffer,
    VkFramebuffer image_framebuffer, VkExtent2D image_extent,
    uint32_t num_clear_values, const VkClearValue clear_values[],
    VkRenderPass render_pass, VkDescriptorSet descriptor_set, VkPipelineLayout pipeline_layout, VkPipeline pipeline
);

void end_pipeline(VkCommandBuffer command_buffer);
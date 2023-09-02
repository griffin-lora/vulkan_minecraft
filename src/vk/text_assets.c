#include "text_assets.h"
#include "core.h"
#include "defaults.h"
#include "vk/gfx_core.h"
#include <malloc.h>
#include <string.h>
#include <stdalign.h>

alignas(64)
text_glyph_render_info_t text_glyph_render_info = { 0 };
text_glyph_allocation_info_t text_glyph_allocation_info = { 0 };

alignas(64) text_model_render_info_t text_model_render_infos[NUM_TEXT_MODELS] = { 0 };
alignas(64) text_model_allocation_info_t text_model_allocation_infos[NUM_TEXT_MODELS] = { 0 };
alignas(64) static VkBuffer text_model_staging_buffers[NUM_TEXT_MODELS] = { 0 };
alignas(64) static VmaAllocation text_model_staging_allocations[NUM_TEXT_MODELS] = { 0 };

alignas(64)
static size_t num_text_model_staging_updates = 0;
static VkBuffer text_model_staging_updates[NUM_TEXT_MODELS] = { 0 };

static uint32_t text_glyph_image_width;
static uint32_t text_glyph_image_height;

VkSampler text_texture_image_sampler;

typedef struct {
    staging_t glyph_vertex_staging;
    staging_t glyph_index_staging;
} text_assets_info_t;

text_assets_info_t* info;

const char* begin_text_assets(float max_anistropy, uint32_t num_mip_levels, uint32_t width, uint32_t height) {
    info = memalign(64, sizeof(text_assets_info_t));
    memset(info, 0, sizeof(text_assets_info_t));

    if (vkCreateSampler(device, &(VkSamplerCreateInfo) {
        DEFAULT_VK_SAMPLER,
        .maxAnisotropy = max_anistropy,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .minFilter = VK_FILTER_NEAREST,
        .magFilter = VK_FILTER_NEAREST,
        .anisotropyEnable = VK_FALSE,
        .maxLod = (float)num_mip_levels
    }, NULL, &text_texture_image_sampler) != VK_SUCCESS) {
        return "Failed to create tetxure image sampler\n";
    }

    text_glyph_image_width = width;
    text_glyph_image_height = height;
    
    text_glyph_vertex_t text_glyph_vertices[NUM_TEXT_GLYPH_VERTICES] = {
        { {{ 0.0f, 0.0f }}, {{ 0.0f, 0.0f }} },
        { {{ (float)TEXT_GLYPH_SCREEN_SIZE, 0.0f }}, {{ (float)TEXT_GLYPH_TEXTURE_SIZE/(float)text_glyph_image_width, 0.0f }} },
        { {{ (float)TEXT_GLYPH_SCREEN_SIZE, (float)TEXT_GLYPH_SCREEN_SIZE }}, {{ (float)TEXT_GLYPH_TEXTURE_SIZE/(float)text_glyph_image_width, (float)TEXT_GLYPH_TEXTURE_SIZE/(float)text_glyph_image_height }} },
        { {{ 0.0f, (float)TEXT_GLYPH_SCREEN_SIZE }}, {{ 0.0f, (float)TEXT_GLYPH_TEXTURE_SIZE/(float)text_glyph_image_height }} }
    };
    
    uint16_t text_glyph_indices[NUM_TEXT_GLYPH_INDICES] = {
        0, 1, 2, 2, 3, 0
    };
    
    if (begin_buffer(&vertex_buffer_create_info, NUM_TEXT_GLYPH_VERTICES, sizeof(text_glyph_vertex_t), text_glyph_vertices, &info->glyph_vertex_staging, &text_glyph_render_info.vertex_buffer, &text_glyph_allocation_info.vertex_allocation) != result_success) {
        return "Failed to begin creating vertex buffer\n";
    }

    if (begin_buffer(&index_buffer_create_info, NUM_TEXT_GLYPH_INDICES, sizeof(uint16_t), text_glyph_indices, &info->glyph_index_staging, &text_glyph_render_info.index_buffer, &text_glyph_allocation_info.index_allocation) != result_success) {
        return "Failed to begin creating index buffer\n";
    }

    return NULL;
}

void transfer_text_assets(VkCommandBuffer command_buffer) {
    vkCmdCopyBuffer(command_buffer, info->glyph_vertex_staging.buffer, text_glyph_render_info.vertex_buffer, 1, &(VkBufferCopy) {
        .size = sizeof(text_glyph_vertex_t)*NUM_TEXT_GLYPH_VERTICES
    });
    vkCmdCopyBuffer(command_buffer, info->glyph_index_staging.buffer, text_glyph_render_info.index_buffer, 1, &(VkBufferCopy) {
        .size = sizeof(uint16_t)*NUM_TEXT_GLYPH_INDICES
    });
}

void end_text_assets(void) {
    vmaDestroyBuffer(allocator, info->glyph_vertex_staging.buffer, info->glyph_vertex_staging.allocation);
    vmaDestroyBuffer(allocator, info->glyph_index_staging.buffer, info->glyph_index_staging.allocation);

    free(info);
}

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

    uint32_t num_characters = (uint32_t)strlen(message);

    uint32_t num_glyphs = num_characters;
    for (uint32_t i = 0; i < num_characters; i++) {
        if (message[i] == '\n') {
            num_glyphs--;
        }
    }

    text_glyph_instance_t instances[num_glyphs];
    uint32_t instance_index = 0;
    vec2s glyph_position = (vec2s) {{ 0.0f, 0.0f }};
    for (uint32_t i = 0; i < num_characters; i++) {
        if (message[i] == '\n') {
            glyph_position = (vec2s) {{ 0.0f, glyph_position.y + TEXT_GLYPH_SCREEN_SIZE }};
            continue;
        }

        int32_t glyph_char = message[i];
        
        vec2s glyph_tex_coord = {{ (float)(glyph_char*TEXT_GLYPH_TEXTURE_SIZE % (int32_t)text_glyph_image_width), (float)(TEXT_GLYPH_TEXTURE_SIZE*(glyph_char*TEXT_GLYPH_TEXTURE_SIZE / (int32_t)text_glyph_image_width)) }};
        glyph_tex_coord.x /= (float)text_glyph_image_width;
        glyph_tex_coord.y /= (float)text_glyph_image_height;

        instances[instance_index++] = (text_glyph_instance_t) {
            .position = glyph_position,
            .tex_coord = glyph_tex_coord,
        };

        glyph_position.x += TEXT_GLYPH_SCREEN_SIZE;
    }

    if (write_to_buffer(text_model_staging_allocations[index], sizeof(instances), instances) != result_success) {
        return result_failure;
    }

    text_model_render_infos[index].num_instances = num_glyphs;
    text_model_staging_updates[num_text_model_staging_updates++] = text_model_staging_buffers[index];
    
    return result_success;
}

void update_text_assets(VkCommandBuffer command_buffer) {
    for (size_t i = 0; i < num_text_model_staging_updates; i++) {
        const text_model_render_info_t* render_info = &text_model_render_infos[i];
        VkDeviceSize num_bytes = render_info->num_instances * sizeof(text_glyph_instance_t);
        
        vkCmdCopyBuffer(command_buffer, text_model_staging_updates[i], render_info->instance_buffer, 1, &(VkBufferCopy) {
            .size = num_bytes
        });

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 1, &(VkBufferMemoryBarrier) {
            DEFAULT_VK_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            .buffer = render_info->instance_buffer,
            .size = num_bytes
        }, 0, NULL);
    }

    num_text_model_staging_updates = 0;
}

void term_text_assets(void) {
    vkDestroySampler(device, text_texture_image_sampler, NULL);

    for (size_t i = 0; i < NUM_TEXT_MODELS; i++) {
        if (text_model_render_infos[i].instance_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, text_model_render_infos[i].instance_buffer, text_model_allocation_infos[i].instance_allocation);
            vmaDestroyBuffer(allocator, text_model_staging_buffers[i], text_model_staging_allocations[i]);
        }
    }

    vmaDestroyBuffer(allocator, text_glyph_render_info.vertex_buffer, text_glyph_allocation_info.vertex_allocation);
    vmaDestroyBuffer(allocator, text_glyph_render_info.index_buffer, text_glyph_allocation_info.index_allocation);
}
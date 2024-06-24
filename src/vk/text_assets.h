#pragma once
#include "result.h"
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <cglm/struct/vec2.h>

typedef struct {
    vec2s position;
    vec2s tex_coord;
} text_glyph_instance_t;

typedef struct {
    vec2s position;
    vec2s tex_coord;
} text_glyph_vertex_t;

typedef struct {
    VkBuffer vertex_buffer;
    VkBuffer index_buffer;
} text_glyph_render_info_t;

typedef struct {
    VmaAllocation vertex_allocation;
    VmaAllocation index_allocation;
} text_glyph_allocation_info_t;

typedef struct {
    vec2s model_position;
    VkBuffer instance_buffer;
    uint32_t num_instances;
} text_model_render_info_t;

typedef struct {
    VmaAllocation instance_allocation;
} text_model_allocation_info_t;

#define NUM_TEXT_GLYPH_VERTICES 4
#define NUM_TEXT_GLYPH_INDICES 6

extern text_glyph_render_info_t text_glyph_render_info;
extern text_glyph_allocation_info_t text_glyph_allocation_info;

#define NUM_TEXT_MODELS 1

extern text_model_render_info_t text_model_render_infos[NUM_TEXT_MODELS];
extern text_model_allocation_info_t text_model_allocation_infos[NUM_TEXT_MODELS];

extern VkSampler text_texture_image_sampler;

#define TEXT_GLYPH_SCREEN_SIZE 24
#define TEXT_GLYPH_TEXTURE_SIZE 8

result_t init_text_model(size_t index, vec2s model_position, uint32_t num_glyphs);
// NOTE: Can only be called once per frame per index
result_t set_text_model_message(size_t index, const char* message);

result_t begin_text_assets(float max_anistropy, uint32_t num_mip_levels, uint32_t width, uint32_t height);
void transfer_text_assets(VkCommandBuffer command_buffer);
void end_text_assets(void);

void update_text_assets(VkCommandBuffer command_buffer);
void term_text_assets(void);
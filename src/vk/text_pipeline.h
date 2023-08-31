#pragma once
#include "result.h"
#include "vk.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
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
    uint32_t num_indices;
} text_glyph_render_info_t;

typedef struct {
    VmaAllocation vertex_allocation;
    VmaAllocation index_allocation;
} text_glyph_allocation_info_t;

extern text_glyph_render_info_t text_glyph_render_info;
extern text_glyph_allocation_info_t text_glyph_allocation_info;

result_t init_text_model(size_t index, vec2s model_position, uint32_t num_glyphs);
// NOTE: Can only be called once per frame per index
result_t set_text_model_message(size_t index, const char* message);

const char* init_text_pipeline(void);
void transfer_text_pipeline(VkCommandBuffer command_buffer);
void draw_text_pipeline(VkCommandBuffer command_buffer);
void term_text_pipeline(void);
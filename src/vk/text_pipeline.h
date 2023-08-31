#pragma once
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

typedef struct {
    vec2s model_position;
    VkBuffer instance_buffer;
    uint32_t num_instances;
} text_model_render_info_t;

typedef struct {
    VmaAllocation instance_allocation;
} text_model_allocation_info_t;

extern text_glyph_render_info_t text_glyph_render_info;
extern text_glyph_allocation_info_t text_glyph_allocation_info;

#define NUM_TEXT_MODELS 1

extern text_model_render_info_t text_model_render_infos[NUM_TEXT_MODELS];
extern text_model_allocation_info_t text_model_allocation_infos[NUM_TEXT_MODELS];

const char* init_text_pipeline(void);
void draw_text_pipeline(VkCommandBuffer command_buffer);
void term_text_pipeline(void);
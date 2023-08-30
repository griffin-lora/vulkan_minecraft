#pragma once
#include "vk.h"
#include "core.h"
#include "result.h"
#include <vk_mem_alloc.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
#include <assert.h>

typedef struct {
    mat4s view_projection;
    union {
        vec3s camera_position;
        vec4s camera_position_raw;
    };
} voxel_color_pipeline_push_constants_t;
extern voxel_color_pipeline_push_constants_t voxel_color_pipeline_push_constants;
static_assert(sizeof(voxel_color_pipeline_push_constants_t) <= 256, "Push constants must be less than or equal to 256 bytes");

const char* init_voxel_color_pipeline(void);
void draw_voxel_color_pipeline(VkCommandBuffer command_buffer);
void term_voxel_color_pipeline(void);
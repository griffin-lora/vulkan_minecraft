#pragma once
#include "result.h"
#include "vk.h"
#include <stddef.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>

// Used on all render passes
typedef struct {
    vec3s position;
    vec2s tex_coord;
} general_pipeline_vertex_t;

typedef union {
    void* data;
    general_pipeline_vertex_t* general_pipeline_vertices;
} vertex_array_t;

#define NUM_VERTEX_ARRAYS 1
#define GENERAL_PIPELINE_VERTEX_ARRAY_INDEX 0

extern uint32_t num_vertex_bytes_array[NUM_VERTEX_ARRAYS];

typedef struct {
    uint32_t num_vertices;
    uint32_t num_indices;
    vertex_array_t vertex_arrays[NUM_VERTEX_ARRAYS];
    union {
        uint16_t* indices;
        void* indices_data;
    };
} mesh_t;

result_t load_gltf_mesh(const char* path, mesh_t* mesh);

typedef struct {
    vec3s position;
    vec2s tex_coord;
} voxel_face_vertex_t;

typedef struct {
    uint32_t num_vertices;
    uint32_t num_indices;
    union {
        voxel_face_vertex_t* vertices;
        void* vertices_data;
    };
    union {
        uint16_t* indices;
        void* indices_data;
    };
} voxel_face_type_mesh_t;

result_t load_gltf_voxel_face_type_meshes(const char* path, size_t num_faces, const char* face_names[], voxel_face_type_mesh_t faces[]);
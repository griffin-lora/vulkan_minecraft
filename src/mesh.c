#include "mesh.h"
#include <stdio.h>
#include <cgltf.h>
#include <malloc.h>
#include <string.h>
#include <stdalign.h>
#include <string.h>

alignas(64) uint32_t num_vertex_bytes_array[NUM_VERTEX_ARRAYS] = {
    [GENERAL_PIPELINE_VERTEX_ARRAY_INDEX] = sizeof(general_pipeline_vertex_t)
};

result_t load_gltf_mesh(const char* path, mesh_t* mesh) {
    cgltf_options options = { 0 };
    cgltf_data* data = NULL;

    if (cgltf_parse_file(&options, path, &data) != cgltf_result_success) {
        return result_failure;
    }

    if (data->meshes_count != 1) {
        return result_failure;
    }

    const cgltf_mesh* mesh_data = &data->meshes[0];
    if (mesh_data->primitives_count != 1) {
        return result_failure;
    }

    const cgltf_primitive* primitive_data = &mesh_data->primitives[0];

    if (primitive_data->attributes_count != 4) {
        return result_failure;
    }
    if (primitive_data->attributes[0].type != cgltf_attribute_type_position) {
        return result_failure;
    }
    if (primitive_data->attributes[1].type != cgltf_attribute_type_normal) {
        return result_failure;
    }
    if (primitive_data->attributes[2].type != cgltf_attribute_type_tangent) {
        return result_failure;
    }
    if (primitive_data->attributes[3].type != cgltf_attribute_type_texcoord) {
        return result_failure;
    }

    if (cgltf_load_buffers(&options, data, path) != cgltf_result_success) {
        return result_failure;
    }

    cgltf_size num_vertices = primitive_data->attributes[0].data->count;

    vertex_array_t vertex_arrays[NUM_VERTEX_ARRAYS];

    for (size_t i = 0; i < NUM_VERTEX_ARRAYS; i++) {
        vertex_arrays[i].data = memalign(64, num_vertices*num_vertex_bytes_array[i]);
    }

    const vec3s* position_data = primitive_data->attributes[0].data->buffer_view->buffer->data + primitive_data->attributes[0].data->buffer_view->offset;
    const vec2s* tex_coord_data = primitive_data->attributes[3].data->buffer_view->buffer->data + primitive_data->attributes[3].data->buffer_view->offset;

    for (size_t i = 0; i < num_vertices; i++) {
        vertex_arrays[GENERAL_PIPELINE_VERTEX_ARRAY_INDEX].general_pipeline_vertices[i] = (general_pipeline_vertex_t) {
            .position = position_data[i],
            .tex_coord = tex_coord_data[i]
        };
    }

    cgltf_size num_indices = primitive_data->indices->count;
    uint16_t* indices = memalign(64, num_indices*sizeof(uint16_t));
    
    const uint16_t* indices_data = primitive_data->indices->buffer_view->buffer->data + primitive_data->indices->buffer_view->offset;

    memcpy(indices, indices_data, num_indices*sizeof(uint16_t));

    cgltf_free(data);

    *mesh = (mesh_t) {
        .num_vertices = (uint32_t)num_vertices,
        .num_indices = (uint32_t)num_indices,
        .indices = indices
    };
    memcpy(mesh->vertex_arrays, vertex_arrays, sizeof(vertex_arrays));
    return result_success;
}

static result_t load_voxel_face_mesh(const cgltf_mesh* mesh_data, voxel_face_mesh_t* face) {
    if (mesh_data->primitives_count != 1) {
        return result_failure;
    }

    const cgltf_primitive* primitive_data = &mesh_data->primitives[0];

    if (primitive_data->attributes_count != 2) {
        return result_failure;
    }
    if (primitive_data->attributes[0].type != cgltf_attribute_type_position) {
        return result_failure;
    }
    if (primitive_data->attributes[1].type != cgltf_attribute_type_texcoord) {
        return result_failure;
    }

    uint32_t num_vertices = (uint32_t)primitive_data->attributes[0].data->count;
    voxel_vertex_t* vertices = memalign(64, num_vertices*sizeof(voxel_vertex_t));

    const vec3s* position_data = primitive_data->attributes[0].data->buffer_view->buffer->data + primitive_data->attributes[0].data->buffer_view->offset;
    const vec2s* tex_coord_data = primitive_data->attributes[1].data->buffer_view->buffer->data + primitive_data->attributes[1].data->buffer_view->offset;

    for (size_t i = 0; i < num_vertices; i++) {
        vertices[i] = (voxel_vertex_t) {
            .position = position_data[i],
            .tex_coord = tex_coord_data[i]
        };
    }

    uint32_t num_indices = (uint32_t)primitive_data->indices->count;
    uint16_t* indices = memalign(64, num_indices*sizeof(uint16_t));

    const uint16_t* indices_data = primitive_data->indices->buffer_view->buffer->data + primitive_data->indices->buffer_view->offset;

    memcpy(indices, indices_data, num_indices*sizeof(uint16_t));
    
    *face = (voxel_face_mesh_t) {
        .num_vertices = num_vertices,
        .num_indices = num_indices,
        .vertices = vertices,
        .indices = indices
    };
    return result_success;
}

result_t load_gltf_voxel_face_meshes(const char* path, size_t num_faces, const char* face_names[], voxel_face_mesh_t faces[]) {
    cgltf_options options = { 0 };
    cgltf_data* data = NULL;

    if (cgltf_parse_file(&options, path, &data) != cgltf_result_success) {
        return result_failure;
    }

    if (cgltf_load_buffers(&options, data, path) != cgltf_result_success) {
        return result_failure;
    }

    for (size_t i = 0; i < data->nodes_count; i++) {
        const cgltf_node* node_data = &data->nodes[i];

        bool found = false;
        for (size_t j = 0; j < num_faces; j++) {
            if (strcmp(node_data->name, face_names[j]) != 0) {
                continue;
            }

            if (load_voxel_face_mesh(node_data->mesh, &faces[j]) != result_success) {
                return result_failure;
            }
            found = true;
            break;
        }

        if (!found) {
            return result_failure;
        }
    }

    cgltf_free(data);

    return result_success;
}
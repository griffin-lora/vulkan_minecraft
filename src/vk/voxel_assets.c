#include "voxel_assets.h"
#include "voxel/face_instance.h"
#include "voxel/region.h"
#include "core.h"
#include "defaults.h"
#include <stdalign.h>
#include <malloc.h>
#include <string.h>

alignas(64) voxel_face_type_render_info_t voxel_face_type_render_infos[NUM_VOXEL_FACE_TYPES] = { 0 };
alignas(64) voxel_face_type_allocation_info_t voxel_face_type_allocation_infos[NUM_VOXEL_FACE_TYPES] = { 0 };

alignas(64) voxel_region_render_info_t voxel_region_render_infos[NUM_VOXEL_REGIONS] = { 0 };
alignas(64) voxel_face_model_render_info_t voxel_face_model_render_info_arrays[NUM_VOXEL_FACE_TYPES][NUM_VOXEL_REGIONS] = { 0 };
alignas(64) voxel_face_model_allocation_info_t voxel_face_model_allocation_info_arrays[NUM_VOXEL_FACE_TYPES][NUM_VOXEL_REGIONS];

VkSampler voxel_region_texture_image_sampler;

typedef struct {
    uint32_t num_face_vertices_array[NUM_VOXEL_FACE_TYPES];

    staging_t face_vertex_stagings[NUM_VOXEL_FACE_TYPES];
    staging_t face_index_stagings[NUM_VOXEL_FACE_TYPES];

    staging_t voxel_face_model_staging_arrays[NUM_VOXEL_FACE_TYPES][NUM_VOXEL_REGIONS];
} voxel_assets_info_t;

static voxel_assets_info_t* info;

const char* begin_voxel_assets(float max_anistropy, uint32_t num_mip_levels) {
    info = memalign(64, sizeof(voxel_assets_info_t));
    memset(info, 0, sizeof(voxel_assets_info_t));

    if (vkCreateSampler(device, &(VkSamplerCreateInfo) {
        DEFAULT_VK_SAMPLER,
        .maxAnisotropy = max_anistropy,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .minFilter = VK_FILTER_NEAREST,
        .magFilter = VK_FILTER_NEAREST,
        .anisotropyEnable = VK_FALSE,
        .maxLod = (float)num_mip_levels
    }, NULL, &voxel_region_texture_image_sampler) != VK_SUCCESS) {
        return "Failed to create tetxure image sampler\n";
    }

    const char* face_mesh_paths[] = {
        "mesh/cube_voxel.glb"
    };

    voxel_face_type_mesh_t cube_face_type_meshes[NUM_VOXEL_FACE_TYPES];

    if (load_gltf_voxel_face_type_meshes(face_mesh_paths[0], NUM_VOXEL_FACE_TYPES, (const char*[]) { "Front", "Back", "Top", "Bottom", "Right", "Left" }, cube_face_type_meshes) != result_success) {
        return "Failed to load mesh\n";
    }

    for (size_t i = 0; i < NUM_VOXEL_FACE_TYPES; i++) {
        const voxel_face_type_mesh_t* mesh = &cube_face_type_meshes[i];
        voxel_face_type_render_info_t* render_info = &voxel_face_type_render_infos[i];
        voxel_face_type_allocation_info_t* allocation_info = &voxel_face_type_allocation_infos[i];

        info->num_face_vertices_array[i] = mesh->num_vertices;
        render_info->num_indices = mesh->num_indices;

        if (begin_buffer(&vertex_buffer_create_info, mesh->num_vertices, sizeof(voxel_face_vertex_t), mesh->vertices, &info->face_vertex_stagings[i], &render_info->vertex_buffer, &allocation_info->vertex_allocation) != result_success) {
            return "Failed to begin creating vertex buffers\n"; 
        }

        if (begin_buffer(&index_buffer_create_info, mesh->num_indices, sizeof(uint16_t), mesh->indices, &info->face_index_stagings[i], &render_info->index_buffer, &allocation_info->index_allocation) != result_success) {
            return "Failed to begin creating index buffer\n";
        }

        free(mesh->vertices);
        free(mesh->indices);
    }

    #define NUM_XZ_VOXEL_REGIONS 8

    voxel_region_voxel_type_array_t (*voxel_type_arrays)[NUM_XZ_VOXEL_REGIONS] = memalign(64, NUM_VOXEL_REGIONS*sizeof(voxel_region_voxel_type_array_t));
    memset(voxel_type_arrays, voxel_type_air, NUM_VOXEL_REGIONS*sizeof(voxel_region_voxel_type_array_t));

    {
        size_t i = 0;
        for (size_t x = 0; x < NUM_XZ_VOXEL_REGIONS; x++)
        for (size_t z = 0; z < NUM_XZ_VOXEL_REGIONS; z++, i++) {
            voxel_region_render_info_t* render_info = &voxel_region_render_infos[i];

            render_info->position = (vec3s) {{ (float)(x * VOXEL_REGION_SIZE), 0.0f, (float)(z * VOXEL_REGION_SIZE) }};

            voxel_region_voxel_type_array_t* voxel_types = &voxel_type_arrays[x][z];

            create_voxel_region_voxel_type_array(x*VOXEL_REGION_SIZE, 0, z*VOXEL_REGION_SIZE, voxel_types);
        }

        i = 0;
        for (size_t x = 0; x < NUM_XZ_VOXEL_REGIONS; x++)
        for (size_t z = 0; z < NUM_XZ_VOXEL_REGIONS; z++, i++) {
            voxel_face_instance_arrays_t face_instance_arrays = { 0 };
            create_voxel_face_instance_arrays(&(voxel_region_voxel_type_arrays_t) {
                .center = &voxel_type_arrays[x][z],
                .front = x + 1u < NUM_XZ_VOXEL_REGIONS ? &voxel_type_arrays[x + 1][z] : NULL,
                .back = x - 1u < NUM_XZ_VOXEL_REGIONS ? &voxel_type_arrays[x - 1][z] : NULL,
                .right = z + 1u < NUM_XZ_VOXEL_REGIONS ? &voxel_type_arrays[x][z + 1] : NULL,
                .left = z - 1u < NUM_XZ_VOXEL_REGIONS ? &voxel_type_arrays[x][z - 1] : NULL,
            }, &face_instance_arrays);

            for (size_t j = 0; j < NUM_VOXEL_FACE_TYPES; j++) {
                if (begin_voxel_face_model_info(&face_instance_arrays.arrays[j], &info->voxel_face_model_staging_arrays[j][i], &voxel_face_model_render_info_arrays[j][i], &voxel_face_model_allocation_info_arrays[j][i]) != result_success) {
                    return "Failed to begin creating voxel region info\n";
                }
            }
        }
    }

    #undef VOXEL_REGIONS_SIZE

    free(voxel_type_arrays);

    return NULL;
}

void transfer_voxel_assets(VkCommandBuffer command_buffer) {
    for (size_t i = 0; i < NUM_VOXEL_FACE_TYPES; i++) {
        const voxel_face_type_render_info_t* render_info = &voxel_face_type_render_infos[i];

        vkCmdCopyBuffer(command_buffer, info->face_vertex_stagings[i].buffer, render_info->vertex_buffer, 1, &(VkBufferCopy) {
            .size = sizeof(voxel_face_vertex_t)*info->num_face_vertices_array[i]
        });
        vkCmdCopyBuffer(command_buffer, info->face_index_stagings[i].buffer, render_info->index_buffer, 1, &(VkBufferCopy) {
            .size = sizeof(uint16_t)*render_info->num_indices
        });

        const voxel_face_model_render_info_t* model_render_infos = voxel_face_model_render_info_arrays[i];
        const staging_t* model_stagings = info->voxel_face_model_staging_arrays[i];

        for (size_t j = 0; j < NUM_VOXEL_REGIONS; j++) {
            const voxel_face_model_render_info_t* model_render_info = &model_render_infos[j];

            if (model_render_info->num_instances != 0) {
                vkCmdCopyBuffer(command_buffer, model_stagings[j].buffer, model_render_info->instance_buffer, 1, &(VkBufferCopy) {
                    .size = sizeof(voxel_face_instance_t)*model_render_info->num_instances
                });
            }
        }
    }
}

void end_voxel_assets() {
    for (size_t i = 0; i < NUM_VOXEL_FACE_TYPES; i++) {
        vmaDestroyBuffer(allocator, info->face_vertex_stagings[i].buffer, info->face_vertex_stagings[i].allocation);
        vmaDestroyBuffer(allocator, info->face_index_stagings[i].buffer, info->face_index_stagings[i].allocation);

        const staging_t* model_statings = info->voxel_face_model_staging_arrays[i];

        for (size_t j = 0; j < NUM_VOXEL_REGIONS; j++) {
            const staging_t* staging = &model_statings[j];
            vmaDestroyBuffer(allocator, staging->buffer, staging->allocation);
        }
    }

    free(info);
}

void term_voxel_assets(void) {
    vkDestroySampler(device, voxel_region_texture_image_sampler, NULL);

    for (size_t i = 0; i < NUM_VOXEL_FACE_TYPES; i++) {
        const voxel_face_type_render_info_t* render_info = &voxel_face_type_render_infos[i];
        const voxel_face_type_allocation_info_t* allocation_info = &voxel_face_type_allocation_infos[i];

        vmaDestroyBuffer(allocator, render_info->vertex_buffer, allocation_info->vertex_allocation);
        vmaDestroyBuffer(allocator, render_info->index_buffer, allocation_info->index_allocation);

        const voxel_face_model_render_info_t* model_render_infos = voxel_face_model_render_info_arrays[i];
        const voxel_face_model_allocation_info_t* model_allocation_infos = voxel_face_model_allocation_info_arrays[i];

        for (size_t j = 0; j < NUM_VOXEL_REGIONS; j++) {
            const voxel_face_model_render_info_t* model_render_info = &model_render_infos[j];
            const voxel_face_model_allocation_info_t* model_allocation_info = &model_allocation_infos[j];

            if (model_render_info->num_instances != 0) {
                vmaDestroyBuffer(allocator, model_render_info->instance_buffer, model_allocation_info->instance_allocation);
            }
        }
    }
}
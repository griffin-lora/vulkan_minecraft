#include "voxel_assets.h"
#include "core.h"
#include "defaults.h"
#include "result.h"
#include <stdalign.h>
#include <malloc.h>
#include <string.h>

VkSampler voxel_region_texture_image_sampler;

result_t begin_voxel_assets(float max_anistropy, uint32_t num_mip_levels) {
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
        return result_sampler_create_failure;
    }

    return result_success;
}

void transfer_voxel_assets(VkCommandBuffer) {
}

void end_voxel_assets(void) {
}

void term_voxel_assets(void) {
    vkDestroySampler(device, voxel_region_texture_image_sampler, NULL);
}
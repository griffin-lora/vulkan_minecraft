#include "result.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

static const char* get_result_string(result_t result) {
    switch (result) {
        case result_failure: return "Generic failure";
        
        case result_instance_create_failure: return "Failed to create instance\n";
        case result_window_surface_create_failure: return "Failed to create window surface\n";
        case result_logical_device_create_failure: return "Failed to create logical device\n";
        case result_memory_allocator_create_failure: return "Failed to create memory allocator\n";
        case result_synchronization_primitive_create_failure: return "Failed to create synchronization primitives\n";
        case result_swapchain_create_failure: return "Failed to create swap chain\n";
        case result_image_view_create_failure: return "Failed to create image view\n";
        case result_framebuffer_create_failure: return "Failed to create framebuffer\n";
        case result_buffer_create_failure: return "Failed to create buffer\n";
        case result_image_create_failure: return "Failed to create image\n";
        case result_sampler_create_failure: return "Failed to create texture image sampler\n";
        case result_render_pass_create_failure: return "Failed to create frame render pass\n";
        case result_descriptor_set_layout_create_failure: return "Failed to create descriptor set layout\n";
        case result_descriptor_pool_create_failure: return "Failed to create descriptor pool\n";
        case result_pipeline_layout_create_failure: return "Failed to create pipeline layout\n";
        case result_shader_module_create_failure: return "Failed to create shader module\n";
        case result_graphics_pipelines_create_failure: return "Failed to create graphics pipelines\n";

        case result_descriptor_sets_allocate_failure: return "Failed to allocate descriptor sets\n";

        case result_command_buffers_allocate_failure: return "Failed to allocate command buffers\n";
        case result_command_buffer_begin_failure: return "Failed to begin command buffer\n"; 
        case result_command_buffer_end_failure: return "Failed to end command buffer\n"; 

        case result_image_pixels_load_failure: return "Failed to load image pixels\n";
        case result_map_memory_failure: return "Failed to map buffer memory\n";

        case result_file_access_failure: return "Failed to access file\n";
        case result_file_open_failure: return "Failed to open file\n";
        case result_file_read_failure: return "Failed to read file\n";

        case result_validation_layers_unavailable: return "Validation layers requested, but not available\n";
        case result_physical_device_support_unavailable: return "Failed to find physical devices with Vulkan support\n";
        case result_suitable_physical_device_unavailable: return "Failed to get a suitable physical device\n";
        case result_supported_depth_image_format_unavailable: return "Failed to get a supported depth image format\n";

        case result_text_model_index_invalid: return "Invalid text model index\n";
        case result_image_dimensions_invalid: return "Invalid image dimensions\n";

        default: return NULL;
    }
}

void print_result_error(result_t result) {
    printf("%s\n", get_result_string(result));
}

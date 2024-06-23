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

        case result_validation_layers_unavailable: return "Validation layers requested, but not available\n";
        case result_physical_device_support_unavailable: return "Failed to find physical devices with Vulkan support\n";
        case result_suitable_physical_device_unavailable: return "Failed to get a suitable physical device\n";
        case result_supported_depth_image_format_unavailable: return "Failed to get a supported depth image format\n";

        case result_text_model_index_invalid: return "Invalid text model index\n";
        default: return NULL;
    }
}

void print_result_error(result_t result) {
    printf("%s\n", get_result_string(result));
}

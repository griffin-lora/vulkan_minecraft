#pragma once

typedef enum {
    result_success = 0,
    result_failure = 1,

    result_instance_create_failure,
    result_window_surface_create_failure,
    result_logical_device_create_failure,
    result_memory_allocator_create_failure,
    result_synchronization_primitive_create_failure,
    result_swapchain_create_failure,
    result_image_view_create_failure,
    result_framebuffer_create_failure,

    result_buffer_create_failure,
    
    result_validation_layers_unavailable,
    result_physical_device_support_unavailable,
    result_suitable_physical_device_unavailable,
    result_supported_depth_image_format_unavailable,

    result_text_model_index_invalid
} result_t;

void print_result_error(result_t result);
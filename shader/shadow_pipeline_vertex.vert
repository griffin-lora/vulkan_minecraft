#version 450

layout(binding = 0) uniform shadow_uniform_constants_t {
    mat4 shadow_view_projection;
};

layout(location = 0) in mat4 model;
layout(location = 4) in vec3 position;

void main() {
	gl_Position = shadow_view_projection * model * vec4(position, 1.0);
}
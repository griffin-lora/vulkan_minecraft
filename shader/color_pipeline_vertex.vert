#version 450

layout(push_constant, std430) uniform push_constants_t {
    mat4 view_projection;
    vec3 camera_position;
};

layout(location = 0) in mat4 model;
layout(location = 4) in vec3 position;
layout(location = 5) in vec2 tex_coord;

layout(location = 0) out vec3 frag_tex_coord;

void main() {
	gl_Position = view_projection * model * vec4(position, 1.0);

	frag_tex_coord = vec3(tex_coord, 0.0);
}
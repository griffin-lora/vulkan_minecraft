#version 450

layout(push_constant, std430) uniform push_constants_t {
    vec2 model_position;
};

layout(location = 0) in vec2 instance_position;
layout(location = 1) in vec2 instance_tex_coord;
layout(location = 2) in vec2 vertex_position;
layout(location = 3) in vec2 vertex_tex_coord;

layout(location = 0) out vec2 frag_tex_coord;

void main() {
	gl_Position = vec4(model_position + instance_position + vertex_position, 0.0, 1.0);
	frag_tex_coord = instance_tex_coord + vertex_tex_coord;
}
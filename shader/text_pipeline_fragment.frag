#version 450

layout(binding = 0) uniform sampler2D color_sampler;

layout(location = 0) in vec2 frag_tex_coord;

layout(location = 0) out vec4 color;

void main() {
	vec3 base_color = texture(color_sampler, frag_tex_coord).rgb;

    color = vec4(base_color, 1.0);
}
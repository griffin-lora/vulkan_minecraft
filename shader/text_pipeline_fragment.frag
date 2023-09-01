#version 450

layout(binding = 0) uniform sampler2D color_sampler;

layout(location = 0) in vec2 frag_tex_coord;

layout(location = 0) out vec4 color;

void main() {
    if (texture(color_sampler, frag_tex_coord).a < 1.0) {
        discard;
    }
    color = vec4(1.0);
}
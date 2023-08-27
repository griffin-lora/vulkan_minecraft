#version 450

layout(push_constant, std430) uniform push_constants_t {
    mat4 view_projection;
    vec3 camera_position;
};

// layout(binding = 0) uniform uniform_constants_t {
//     mat4 model;
// };

layout(location = 0) in vec3 face_position;
layout(location = 1) in vec3 vertex_position;
layout(location = 2) in vec2 tex_coord;

layout(location = 0) out vec3 frag_tex_coord;

void main() {
    mat4 model = mat4(1.0);

	gl_Position = view_projection * model * vec4(face_position + vertex_position, 1.0);

	frag_tex_coord = vec3(tex_coord, 0.0);
}
#version 450

layout(push_constant, std430) uniform push_constants_t {
    mat4 view_projection;
    vec3 camera_position;
};

// layout(binding = 0) uniform uniform_constants_t {
//     vec3 face_model_position;
// };

layout(location = 0) in uint face_instance_info;
layout(location = 1) in vec3 vertex_position;
layout(location = 2) in vec2 tex_coord;

layout(location = 0) out vec3 frag_tex_coord;

void main() {
    vec3 face_model_position = vec3(0.0, 0.0, 0.0);

    uint x = face_instance_info & 0x000000ffu;
    uint y = (face_instance_info & 0x0000ff00u) >> 1u*8u;
    uint z = (face_instance_info & 0x00ff0000u) >> 2u*8u;
    uint texture_array_index = (face_instance_info & 0xff000000u) >> 3u*8u;

    vec3 face_instance_position = vec3(x, y, z);

	gl_Position = view_projection * vec4(face_instance_position + face_model_position + vertex_position, 1.0);

	frag_tex_coord = vec3(tex_coord, texture_array_index);
}
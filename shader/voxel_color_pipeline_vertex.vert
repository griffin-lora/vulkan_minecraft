#version 450

#define GET_BITMASK(NUM_BITS) ((1u << (NUM_BITS)) - 1u)

#define NUM_TEXTURE_ARRAY_INDEX_BITS 8u
#define NUM_FACE_TYPE_BITS 3u
#define NUM_VOXEL_REGION_AXIS_BITS 6u

uint texture_array_index_bitmask = GET_BITMASK(NUM_TEXTURE_ARRAY_INDEX_BITS);
uint face_type_bitmask = GET_BITMASK(NUM_FACE_TYPE_BITS);
uint axis_bitmask = GET_BITMASK(NUM_VOXEL_REGION_AXIS_BITS);

layout(push_constant, std430) uniform push_constants_t {
    mat4 view_projection;
    vec4 region_position;
};

layout(location = 0) in uint face_instance_info;
layout(location = 1) in vec3 vertex_position;
layout(location = 2) in vec2 tex_coord;

layout(location = 0) out vec3 frag_tex_coord;

void main() {
    uint texture_array_index = face_instance_info & texture_array_index_bitmask;
    uint face_type = (face_instance_info >> NUM_TEXTURE_ARRAY_INDEX_BITS) & face_type_bitmask;

    uint x = (face_instance_info >> (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_TYPE_BITS)) & axis_bitmask;
    uint y = (face_instance_info >> (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_TYPE_BITS + 1u*NUM_VOXEL_REGION_AXIS_BITS)) & axis_bitmask;
    uint z = (face_instance_info >> (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_TYPE_BITS + 2u*NUM_VOXEL_REGION_AXIS_BITS)) & axis_bitmask;

    vec3 face_instance_position = vec3(x, y, z);

	gl_Position = view_projection * vec4(face_instance_position + region_position.xyz + vertex_position, 1.0);

	frag_tex_coord = vec3(tex_coord, texture_array_index);
}
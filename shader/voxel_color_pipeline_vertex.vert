#version 450

#define GET_BITMASK(NUM_BITS) ((1u << (NUM_BITS)) - 1u)

#define NUM_TEXTURE_ARRAY_INDEX_BITS 8u
#define NUM_FACE_BITS 3u
#define NUM_VOXEL_REGION_AXIS_BITS 6u

uint texture_array_index_bitmask = GET_BITMASK(NUM_TEXTURE_ARRAY_INDEX_BITS);
uint face_bitmask = GET_BITMASK(NUM_FACE_BITS);
uint axis_bitmask = GET_BITMASK(NUM_VOXEL_REGION_AXIS_BITS);

#define NUM_CUBE_VOXEL_VERTICES 36u
#define NUM_CUBE_VOXEL_FACE_VERTICES 6u

vec3 cube_positions[NUM_CUBE_VOXEL_VERTICES] = {
    vec3(1.000000, 1.000000, -0.000000),
    vec3(1.000000, 0.000000, -0.000000),
    vec3(1.000000, 0.000000, -1.000000),
    vec3(1.000000, 1.000000, -0.000000),
    vec3(1.000000, 0.000000, -1.000000),
    vec3(1.000000, 1.000000, -1.000000),
    vec3(0.000000, 0.000000, -0.000000),
    vec3(-0.000000, 1.000000, -0.000000),
    vec3(-0.000000, 1.000000, -1.000000),
    vec3(0.000000, 0.000000, -0.000000),
    vec3(-0.000000, 1.000000, -1.000000),
    vec3(0.000000, 0.000000, -1.000000),
    vec3(0.000000, 1.000000, -0.000000),
    vec3(1.000000, 1.000000, -0.000000),
    vec3(1.000000, 1.000000, -1.000000),
    vec3(0.000000, 1.000000, -0.000000),
    vec3(1.000000, 1.000000, -1.000000),
    vec3(0.000000, 1.000000, -1.000000),
    vec3(0.000000, 0.000000, -1.000000),
    vec3(1.000000, 0.000000, -1.000000),
    vec3(1.000000, -0.000000, -0.000000),
    vec3(0.000000, 0.000000, -1.000000),
    vec3(1.000000, -0.000000, -0.000000),
    vec3(0.000000, -0.000000, -0.000000),
    vec3(0.000000, 0.000000, -0.000000),
    vec3(1.000000, 0.000000, -0.000000),
    vec3(1.000000, 1.000000, 0.000000),
    vec3(0.000000, 0.000000, -0.000000),
    vec3(1.000000, 1.000000, 0.000000),
    vec3(0.000000, 1.000000, 0.000000),
    vec3(0.000000, 1.000000, -1.000000),
    vec3(1.000000, 1.000000, -1.000000),
    vec3(1.000000, 0.000000, -1.000000),
    vec3(0.000000, 1.000000, -1.000000),
    vec3(1.000000, 0.000000, -1.000000),
    vec3(0.000000, 0.000000, -1.000000)
};

vec2 cube_tex_coords[NUM_CUBE_VOXEL_VERTICES] = {
    vec2(1.000000, 0.000000),
    vec2(1.000000, 1.000000),
    vec2(-0.000000, 1.000000),
    vec2(1.000000, 0.000000),
    vec2(-0.000000, 1.000000),
    vec2(0.000000, 0.000000),
    vec2(-0.000000, 1.000000),
    vec2(0.000000, 0.000000),
    vec2(1.000000, 0.000000),
    vec2(-0.000000, 1.000000),
    vec2(1.000000, 0.000000),
    vec2(1.000000, 1.000000),
    vec2(0.000000, 0.000000),
    vec2(1.000000, 0.000000),
    vec2(1.000000, 1.000000),
    vec2(0.000000, 0.000000),
    vec2(1.000000, 1.000000),
    vec2(0.000000, 1.000000),
    vec2(0.000000, 0.000000),
    vec2(1.000000, 0.000000),
    vec2(1.000000, 1.000000),
    vec2(0.000000, 0.000000),
    vec2(1.000000, 1.000000),
    vec2(0.000000, 1.000000),
    vec2(1.000000, 1.000000),
    vec2(0.000000, 1.000000),
    vec2(0.000000, 0.000000),
    vec2(1.000000, 1.000000),
    vec2(0.000000, 0.000000),
    vec2(1.000000, 0.000000),
    vec2(0.000000, 0.000000),
    vec2(1.000000, 0.000000),
    vec2(1.000000, 1.000000),
    vec2(0.000000, 0.000000),
    vec2(1.000000, 1.000000),
    vec2(0.000000, 1.000000)
};

layout(push_constant, std430) uniform push_constants_t {
    mat4 view_projection;
    vec4 region_position;
};

layout(location = 0) in uint vertex_info;

layout(location = 0) out vec3 frag_tex_coord;

void main() {
    uint texture_array_index = vertex_info & texture_array_index_bitmask;
    uint face = (vertex_info >> NUM_TEXTURE_ARRAY_INDEX_BITS) & face_bitmask;

    uint x = (vertex_info >> (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_BITS)) & axis_bitmask;
    uint y = (vertex_info >> (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_BITS + 1u*NUM_VOXEL_REGION_AXIS_BITS)) & axis_bitmask;
    uint z = (vertex_info >> (NUM_TEXTURE_ARRAY_INDEX_BITS + NUM_FACE_BITS + 2u*NUM_VOXEL_REGION_AXIS_BITS)) & axis_bitmask;

    uint cube_vertex_index = (face * NUM_CUBE_VOXEL_FACE_VERTICES) + (gl_VertexIndex % NUM_CUBE_VOXEL_FACE_VERTICES);
    vec3 vertex_position = cube_positions[cube_vertex_index];

    vec3 voxel_position = vec3(x, y, z);

	gl_Position = view_projection * vec4(voxel_position + region_position.xyz + vertex_position, 1.0);

	frag_tex_coord = vec3(cube_tex_coords[cube_vertex_index], texture_array_index);
}
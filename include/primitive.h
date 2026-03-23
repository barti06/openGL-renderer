#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "cglm/types.h"
#include "cgltf.h"

#include "texture.h"

typedef struct Primitive
{
    // openGL handles
    GLuint VAO;
    GLuint VBO_positions;
    GLuint VBO_uvs;
    GLuint VBO_normals;
    GLuint VBO_tangents;
    GLuint EBO;

    // textures
    GLuint albedo;
    GLuint metallic_roughness;
    GLuint normal;
    GLuint emissive;
    GLuint ambient_occlusion;
    GLuint iridescence;
    GLuint iridescence_thickness;

    // textures factors
    vec4 albedo_factor;
    float metallic_factor;
    float roughness_factor;
    vec2 normal_scale;
    float occlusion_strength;
    vec2 occlusion_scale;
    vec3 emissive_factor;
    float emissive_strength;
    float iridescence_thickness_max;
    float iridescence_thickness_min;
    float iridescence_factor;
    float iridescence_ior;

    // num of instances to draw
    GLsizei index_count;
    GLenum  index_type;
} Primitive;

#include <stdalign.h>

typedef struct Mesh
{
    Primitive* primitives;
    uint32_t primitive_count;
    mat4 transform;
} Mesh;

int upload_accessor_float(cgltf_accessor* accessor, GLuint VBO, GLuint attrib_index, GLint num_components);

int upload_accessor_index(cgltf_accessor* accessor, GLuint EBO, GLsizei* out_count);

int primitive_load(Primitive* model_primitive, cgltf_primitive* src, const char* base_path, TextureCache* cache);

void primitive_free(Primitive* model_primitive);

void mesh_free(Mesh* model_mesh);

#endif
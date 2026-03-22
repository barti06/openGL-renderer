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

    // textures factors
    vec4 albedo_factor;
    float metallic_factor;
    float roughness_factor;
    vec3 emissive_factor;
    float emissive_strength;

    // num of instances to draw
    GLsizei index_count;
    GLenum  index_type;
} Primitive;

int upload_accessor_float(cgltf_accessor* accessor, GLuint VBO, GLuint attrib_index, GLint num_components);

int upload_accessor_index(cgltf_accessor* accessor, GLuint EBO, GLsizei* out_count);

int primitive_load(Primitive* model_primitive, cgltf_primitive* src, const char* base_path, TextureCache* cache);

void primitive_free(Primitive* model_primitive);

#endif
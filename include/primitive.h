#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "cglm/types.h"
#include "cgltf.h"
#include "stb_image.h"

#include <glad/glad.h>

#define MAX_MODEL_PATH_LENGTH 512

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

    // textures factors
    vec4 albedo_factor;
    float metallic_factor;
    float roughness_factor;

    // num of instances to draw
    GLsizei index_count;
    GLenum  index_type;
} Primitive;

int upload_accessor_float(cgltf_accessor* accessor, GLuint VBO, GLuint attrib_index, GLint num_components);

int upload_accessor_index(cgltf_accessor* accessor, GLuint EBO, GLsizei* out_count);

int primitive_load(Primitive* model_primitive, cgltf_primitive* src, const char* base_path);

void primitive_free(Primitive* model_primitive);

GLuint albedo_texture_load(cgltf_primitive* primitive, const char* base_path);

GLuint metallic_roughness_texture_load(cgltf_primitive* primitive, const char* base_path);

GLuint normal_texture_load(cgltf_primitive* primitive, const char* base_path);


#endif
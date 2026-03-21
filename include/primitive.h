#ifndef PRIMITIVE_H
#define PRIMITIVE_H

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
    GLuint EBO;
    GLuint albedo;

    // num of instances to draw
    GLsizei index_count;
    GLenum  index_type;
} Primitive;

int upload_accessor_float(cgltf_accessor* accessor, GLuint VBO, GLuint attrib_index, GLint num_components);

int upload_accessor_index(cgltf_accessor* accessor, GLuint EBO, GLsizei* out_count);

int primitive_load(Primitive* model_primitive, cgltf_primitive* src, const char* base_path);

void primitive_free(Primitive* model_primitive);

GLuint albedo_texture_load(cgltf_primitive* primitive, const char* base_path);

#endif
#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <utils.h>
#include <cglm/cglm.h>

typedef struct Shader
{
    const char* vert_path;
    const char* frag_path;
    uint32_t ID;
} Shader;

void shader_init(Shader* shader, const char* vertexPath, const char* fragmentPath);

void shader_reload_frag(Shader* shader);

void shader_use(Shader* shader);

// uniform utility functions
void shader_set_bool(Shader* shader, const char* name, bool value);
void shader_set_int(Shader* shader, const char* name, int value);
void shader_set_float(Shader* shader, const char* name, float value);

// vector uniform functions
void shader_set_vec2(Shader* shader, const char* name, const vec2 value);
void shader_set_vec3(Shader* shader, const char* name, const vec3 value);
void shader_set_vec4(Shader* shader, const char* name, const vec4 value);

// matrix uniform functions
void shader_set_mat3(Shader* shader, const char* name, const mat3 value);
void shader_set_mat4(Shader* shader, const char* name, const mat4 value);

#endif

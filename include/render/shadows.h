#ifndef SHADOWS_H
#define SHADOWS_H

#include <glad/glad.h>
#include <cglm/cglm.h>

typedef struct Shader Shader;

typedef struct
{
    GLuint fbo;
    GLuint tex;

    mat4 lightProj;
    mat4 lightView;
    mat4 lightSpaceMat;

    float shadow_near;
    float shadow_far;

} shadowMap_t;

void shadowMap_init(shadowMap_t *shadowmap);
void shadowMap_pass(Shader *shadowMap_shader, shadowMap_t *shadowmap, vec3 lightDir);
void shadowMap_send(Shader *shader, int unit, shadowMap_t *shadows);

#endif

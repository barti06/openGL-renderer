#ifndef RENDERER_H
#define RENDERER_H

#include "shader.h"
#include "world.h"


typedef struct Renderer
{
    // engine owns most, renderer juts uses them in a given order
    Shader* active_shader;

    vec2 viewportSize;
    float nearZ;
    float farZ;

    // bit of a mess for now
    GLuint fbo;
    GLuint quad_VAO;
    GLuint quad_VBO;
    GLuint quad_tex_color;
    GLuint quad_tex_depth;
    Shader quad_shader;
} Renderer;

void renderer_init(Renderer* renderer, Shader* shader, 
    int viewportX, int viewportY,
    float nearZ, float farZ);
void renderer_draw_world(World* world, Renderer* renderer);
void renderer_updates(World* world, Renderer* renderer, int windowX, int windowY);

#endif

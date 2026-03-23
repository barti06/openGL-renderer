#ifndef RENDERER_H
#define RENDERER_H

#include "shader.h"
#include "world.h"


typedef struct Renderer
{
    // engine owns all shaders, renderer juts uses them in a given order
    Shader* active_shader;

    vec2 viewportSize;
    float nearZ;
    float farZ;
} Renderer;

void renderer_init(Renderer* renderer, Shader* shader, 
    int viewportX, int viewportY,
    float nearZ, float farZ);
void renderer_draw_world(World* world, Renderer* renderer);
void renderer_updates(World* world, Renderer* renderer, int windowX, int windowY);

#endif

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

    GLuint fbo;
    GLuint g_albedo; // albedo @rgb
    GLuint g_orm; // occlusion roughness metalness
    GLuint g_emissive; // idk if i should send emissive maps as separate or albedo so here they are
    GLuint g_normal;
    GLuint g_position;
    GLuint g_depth;

    GLuint geometry_query;
    GLuint light_query;
    float stats_timer;
    float stats_update_interval;
    float stats_geometry_ms;
    float stats_light_ms;
    float stats_fps;

    int32_t gbuffer_view;

    GLuint quad_VAO;
    GLuint quad_VBO;
    Shader quad_shader;
} Renderer;

void renderer_init(Renderer* renderer, Shader* shader, 
    int viewportX, int viewportY,
    float nearZ, float farZ);
void renderer_draw_world(World* world, Renderer* renderer, double delta_time);
void renderer_updates(World* world, Renderer* renderer, int windowX, int windowY);
void renderer_gbuffer_reload(Renderer* renderer);
#endif

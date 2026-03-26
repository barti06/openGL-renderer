#ifndef RENDERER_H
#define RENDERER_H

#include "shader.h"
#include "world.h"
#include "entity.h"

typedef enum
{
    TONEMAP_ACES,
    TONEMAP_REINHARD,
    TONEMAP_FILMIC
} tonemap_t;

typedef enum
{
    GBUFFER_FINAL,
    GBUFFER_POSITION,
    GBUFFER_NORMAL,
    GBUFFER_ALBEDO,
    GBUFFER_OCCLUSION,
    GBUFFER_ROUGHNESS,
    GBUFFER_METALNESS,
    GBUFFER_EMISSIVE,
    GBUFFER_DEPTH
} gbufferView_t;

typedef struct Renderer
{
    // engine will own most shaders, renderer juts uses them in a given order
    Shader* active_shader;

    vec2 viewportSize;
    float nearZ;
    float farZ;

    GLuint gBuffer_fbo;
    GLuint g_albedo; // albedo @rgb
    GLuint g_orm; // occlusion roughness metalness
    GLuint g_emissive; // idk if i should send emissive maps as separate or albedo so here they are
    GLuint g_normal;
    GLuint g_position;
    GLuint g_depth;

    GLuint fx_fbo;
    GLuint fx_scene;

    // for gpu timing
    GLuint geometry_query;
    GLuint light_query;
    GLuint fx_query;

    float stats_timer;
    float stats_update_interval;
    float stats_geometry_ms;
    float stats_light_ms;
    float stats_fx_ms;
    float stats_fps;

    // various rendering related settings
    gbufferView_t gbuffer_view;
    tonemap_t tonemap;
    float gamma;
    float exposure;
    float brightness;
    bool vignette_enabled;
    float vignette_strength;
    bool CA_enabled;
    float CA_strength;

    // the drawing quad + shaders
    GLuint quad_VAO;
    GLuint quad_VBO;
    Shader light_shader; 
    Shader fx_shader;
} Renderer;

void renderer_init(Renderer* renderer, Shader* shader, 
    int viewportX, int viewportY);
void renderer_draw_world(World* world, Renderer* renderer, double delta_time);
void renderer_updates(World* world, Renderer* renderer, int windowX, int windowY);

void renderer_ui(Renderer* renderer);

void renderer_gbuffer_reload(Renderer* renderer);
// camera position is needed for lighting calculations each frame
void renderer_gbuffer_update(Renderer* renderer, vec3 camera_pos);

void renderer_get_light(Renderer* renderer, LightComponent* lc, vec3 position);

void renderer_postfx_reload(Renderer* renderer);
void renderer_postfx_update(Renderer* renderer);
#endif

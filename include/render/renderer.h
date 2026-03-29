#ifndef RENDERER_H
#define RENDERER_H

#include "shader.h"

typedef enum
{
    TONEMAP_ACES,
    TONEMAP_REINHARD,
    TONEMAP_FILMIC,
    TONEMAP_MAX
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
    GBUFFER_DEPTH,
    GBUFFER_MAX
} gbufferView_t;

typedef enum
{
    PIPELINE_FORWARD,
    PIPELINE_DEFERRED,
    PIPELINE_MAX
} pipeline_t;

typedef struct World World;

typedef struct Renderer
{
    Shader* active_shader;
    Shader deferred_shader;
    Shader forward_shader;
    Shader depth_shader; // for depth prepass on fwd rendering

    vec2 viewportSize;
    float nearZ;
    float farZ;

    GLuint gBuffer_fbo;
    GLuint g_albedo; // albedo @rgb
    GLuint g_orm; // occlusion roughness metalness thickness
    GLuint g_emissive; // idk if i should send emissive maps as separate or albedo so here they are
    GLuint g_normal;
    GLuint g_position;
    GLuint g_depth;
    Shader light_shader; 

    GLuint fx_fbo;
    GLuint fx_scene;
    GLuint fx_depth;

    // for gpu timing
    GLuint geometry_query;
    GLuint light_query;
    GLuint fx_query;
    GLuint ssao_query;
    GLuint ssao_blur_query;

    float stats_timer;
    float stats_update_interval;
    float stats_geometry_ms;
    float stats_light_ms;
    float stats_fx_ms;
    float stats_ssao_ms;
    float stats_ssao_blur_ms;
    float stats_fps;

    // various rendering related settings
    pipeline_t render_mode;
    gbufferView_t gbuffer_view;
    tonemap_t tonemap;
    float gamma;
    float exposure;
    float brightness;
    bool vignette_enabled;
    float vignette_strength;
    bool CA_enabled;
    float CA_strength;

    // bloom
    GLuint bloom_fbo[2]; // pingpong fbos
    GLuint fx_bloom[2]; // bloom tex
    GLuint bloom_bright; // scene brightness
    Shader bloom_blur_shader;
    // bloom settings
    int bloom_blur_passes;
    float bloom_threshold;
    float bloom_strength;
    bool bloom_enabled;

    vec3 ssao_kernel[64];
    GLuint ssao_noise_texture;
    GLuint ssao_fbo;
    GLuint ssao_texture; // noisy ao
    GLuint ssao_blur_fbo;
    GLuint ssao_blur_texture; // blurred AO sent to lighting pass
    // ssao settings
    float ssao_radius;
    float ssao_bias;
    float ssao_strength;
    bool ssao_enabled;
    Shader ssao_shader;
    Shader ssao_blur_shader;

    // the drawing quad + shaders
    GLuint quad_VAO;
    GLuint quad_VBO;
    Shader fx_shader;
} Renderer;

void renderer_init(Renderer* renderer, pipeline_t pipeline, 
    int viewportX, int viewportY);
void renderer_draw_world(World* world, Renderer* renderer, double delta_time);
void renderer_updates(World* world, Renderer* renderer, int windowX, int windowY);

void renderer_ui(Renderer* renderer);

void renderer_gbuffer_reload(Renderer* renderer);
// camera position is needed for lighting calculations each frame
void renderer_gbuffer_update(Renderer* renderer, vec3 camera_pos);

void renderer_postfx_reload(Renderer* renderer);
void renderer_postfx_update(Renderer* renderer);
#endif

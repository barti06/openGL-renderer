#ifndef RENDERER_H
#define RENDERER_H

#include "shader.h"
#include "shadows.h"

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
    GBUFFER_BLOOM_NOBLUR,
    GBUFFER_SHADOW,
    GBUFFER_MAX
} gbufferView_t;

typedef struct 
{
    // for gpu timing
    GLuint geometry_query;
    GLuint light_query;
    GLuint fx_query;
    GLuint ssao_query;
    GLuint ssao_blur_query;
    GLuint shadow_query;

    float stats_timer;
    float stats_update_interval;
    float stats_geometry_ms;
    float stats_light_ms;
    float stats_fx_ms;
    float stats_ssao_ms;
    float stats_ssao_blur_ms;
    float stats_shadows_ms;
    float stats_fps;
} renderTimers_t;

typedef struct
{
    GLuint gBuffer_fbo;
    GLuint g_albedo; // albedo @rgb
    GLuint g_orm; // occlusion roughness metalness thickness
    GLuint g_emissive; // idk if i should send emissive maps as separate or albedo so here they are
    GLuint g_normal;
    GLuint g_depth;
    Shader light_shader; 
    Shader deferred_shader;
} gBuffer_t;

typedef enum
{
    IBL_SELECTION_CLOUDS,
    IBL_SELECTION_SNOW,
    IBL_SELECTION_STUDIO,
    IBL_SELECTION_MAX
} iblSelection_t;

typedef struct
{
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
    // bloom settings
    int bloom_blur_passes;
    float bloom_threshold;
    float bloom_strength;
    bool bloom_enabled;
    // ssao settings
    float ssao_radius;
    float ssao_bias;
    float ssao_strength;
    int hbao_directions;
    int hbao_steps;
    bool ssao_enabled;
    // shadow settings
    bool shadows_enabled;
    float shadows_bias;
    float shadows_spread;
    iblSelection_t ibl_selected;
} renderSettings_t;

typedef struct 
{
    // bloom
    GLuint bloom_fbo[2]; // pingpong fbos
    GLuint fx_bloom[2]; // bloom tex
    GLuint bloom_bright; // scene brightness
    Shader bloom_blur_shader;
} bloom_t;

typedef struct
{
    GLuint ssao_noise_texture;
    GLuint ssao_fbo;
    GLuint ssao_texture; // noisy ao
    GLuint ssao_blur_fbo;
    GLuint ssao_blur_texture; // blurred AO sent to lighting pass
    Shader ssao_shader;
    Shader ssao_blur_shader;
} ssao_t;

typedef struct
{
    GLuint fx_fbo;
    GLuint fx_scene;
    GLuint fx_depth;
    Shader fx_shader;
} postEffects_t;

typedef struct
{
    //skybox data
    uint32_t env_cubemap;
    uint32_t irradianceMap;
    uint32_t prefilter_map;
} ibl_t;

typedef struct
{
    uint32_t ibl_fbo, ibl_rbo;
    Shader hdr_equirec;
    Shader hdr_bg;
    Shader irradiance_shader;
    Shader prefilter_shader;
    uint32_t brdf_LUT;
    Shader brdf_shader;
} iblShared_t;

typedef struct World World;

typedef struct
{
    Shader* active_shader;
    gBuffer_t gbuffer;

    vec2 viewportSize;
    float nearZ;
    float farZ;

    postEffects_t fx;

    renderTimers_t timers;

    renderSettings_t settings;

    bloom_t bloom;

    ssao_t ssao;

    
    iblShared_t ibl_shared;
    ibl_t cloud_ibl;
    ibl_t snow_ibl;
    ibl_t studio_ibl;
    ibl_t *active_ibl;

    shadowMap_t shadow;
    Shader shadowMap_shader;

    // the drawing quad + shaders
    GLuint quad_VAO;
    GLuint quad_VBO;

    GLuint cubeVAO;
    GLuint cubeVBO;
} Renderer;

void renderer_init(Renderer* renderer, 
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

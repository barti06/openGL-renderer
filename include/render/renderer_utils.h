#pragma once
#include "renderer.h"
#include <log.h>

static inline void gbuffer_setup(Renderer* renderer, int w, int h)
{
    glGenFramebuffers(1, &renderer->gbuffer.gBuffer_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->gbuffer.gBuffer_fbo);

    // main pass normal texture
    glGenTextures(1, &renderer->gbuffer.g_normal);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->gbuffer.g_normal, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass albedo texture
    glGenTextures(1, &renderer->gbuffer.g_albedo);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_albedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, renderer->gbuffer.g_albedo, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass orm (occlusion, roughness & metalness) texture
    glGenTextures(1, &renderer->gbuffer.g_orm);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_orm);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, renderer->gbuffer.g_orm, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass emissive texture
    glGenTextures(1, &renderer->gbuffer.g_emissive);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_emissive);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, renderer->gbuffer.g_emissive, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass depth texture
    glGenTextures(1, &renderer->gbuffer.g_depth);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderer->gbuffer.g_depth, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint attachments[] = { 
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, 
        GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 
    };
    glDrawBuffers((sizeof(attachments) / sizeof(GLuint)), attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	    log_error("ERROR... renderer_init() says: FRAMEBUFFER INCOMPLETE.\n");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Shader *ds = &renderer->gbuffer.deferred_shader;
    Shader *ls = &renderer->gbuffer.light_shader;
    // init light shader vars
    shader_use(ls);
    shader_set_int(ls, "g_normal", 0);
    shader_set_int(ls, "g_albedo", 1);
    shader_set_int(ls, "g_orm", 2);
    shader_set_int(ls, "g_emissive", 3);
    shader_set_int(ls, "g_depth", 4);
    shader_set_int(ls, "ssao", 5);
    shader_set_int(ls, "u_shadowMap", 6);

    // init gbuffer shader vars
    shader_use(ds);
    shader_set_int(ds, "u_albedo", 0);
    shader_set_int(ds, "u_metallic_roughness", 1);
    shader_set_int(ds, "u_normal", 2);
    shader_set_int(ds, "u_emissive", 3);
    shader_set_int(ds, "u_ao", 4);
    shader_set_int(ds, "u_iridescence", 5);
    shader_set_int(ds, "u_iridescence_thickness", 6);
    shader_set_int(ds, "u_volume_thickness", 7);
}

static inline void postFX_setup(Renderer* renderer, int w, int h)
{
    postEffects_t *fx = &renderer->fx;
    glGenFramebuffers(1, &fx->fx_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fx->fx_fbo);

    // all of gbuffer's lighting calculations
    glGenTextures(1, &fx->fx_scene);
    glBindTexture(GL_TEXTURE_2D, fx->fx_scene);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fx->fx_scene, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // used in forward rendering
    glGenTextures(1, &fx->fx_depth);
    glBindTexture(GL_TEXTURE_2D, fx->fx_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fx->fx_depth, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    bloom_t *b = &renderer->bloom;

    // bloom brightness extractor
    glGenTextures(1, &b->bloom_bright);
    glBindTexture(GL_TEXTURE_2D, b->bloom_bright);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, b->bloom_bright, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint attachments[] = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1
    };
    glDrawBuffers(sizeof(attachments) / sizeof(GLuint), attachments);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong blur fbos
    glGenFramebuffers(2, b->bloom_fbo);
    glGenTextures(2, b->fx_bloom);
    for(int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, b->bloom_fbo[i]);
        glBindTexture(GL_TEXTURE_2D, b->fx_bloom[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, b->fx_bloom[i], 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

static inline void gbuffer_update(Renderer* renderer, int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->gbuffer.gBuffer_fbo);

    // main pass normal texture
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass albedo texture
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_albedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass orm (occlusion, roughness & metalness) texture
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_orm);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass emissive textures
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_emissive);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass depth texture
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static inline void postFX_update(Renderer* renderer, int w, int h)
{
    postEffects_t *fx = &renderer->fx;
    glBindFramebuffer(GL_FRAMEBUFFER, fx->fx_fbo);
    // all of gbuffer's lighting calculations
    glBindTexture(GL_TEXTURE_2D, fx->fx_scene);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // used in forward
    glBindTexture(GL_TEXTURE_2D, fx->fx_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    bloom_t *b = &renderer->bloom;
    // bloom brightness update
    glBindTexture(GL_TEXTURE_2D, b->bloom_bright);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // bloom blur update
    for(int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, b->bloom_fbo[i]);
        glBindTexture(GL_TEXTURE_2D, b->fx_bloom[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

static inline void ssao_setup(Renderer* renderer, int w, int h)
{
    srand(clock());

    // generate 4x4 noise texture (tiled over screen to avoid banding)
    // noise rotates the kernel per-pixel to get more sample variety
    float ssao_noise[64];
    for(int i = 0; i < 64; i++)
        ssao_noise[i] = (float)rand() / RAND_MAX;

    ssao_t *ssao = &renderer->ssao;
    // ssao noise tex
    glGenTextures(1, &ssao->ssao_noise_texture);
    glBindTexture(GL_TEXTURE_2D, ssao->ssao_noise_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, 4, 4, 0, GL_RED, GL_FLOAT, ssao_noise);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // enable tiling
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    // ssao fbo and tex
    glGenFramebuffers(1, &ssao->ssao_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, ssao->ssao_fbo);
    glGenTextures(1, &ssao->ssao_texture);
    glBindTexture(GL_TEXTURE_2D, ssao->ssao_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao->ssao_texture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ssao fbo and tex
    glGenFramebuffers(1, &ssao->ssao_blur_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, ssao->ssao_blur_fbo);
    glGenTextures(1, &ssao->ssao_blur_texture);
    glBindTexture(GL_TEXTURE_2D, ssao->ssao_blur_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao->ssao_blur_texture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    renderSettings_t *rs = &renderer->settings;

    rs->ssao_radius = 0.5f;
    rs->ssao_bias = 0.02f;
    rs->ssao_strength = 2.2f;
    rs->hbao_directions = 4;
    rs->hbao_steps = 4;
    rs->ssao_enabled = true;

    Shader *aos = &ssao->ssao_shader;

    shader_use(aos);
    shader_set_int(aos, "g_normal", 0);
    shader_set_int(aos, "g_depth", 1);
    shader_set_int(aos, "u_noise", 2);
    shader_set_int(aos, "u_directions", rs->hbao_directions);
    shader_set_int(aos, "u_steps", rs->hbao_steps);
    float noise_scale_x = (float)w / 4.0f;
    float noise_scale_y = (float)h / 4.0f;
    shader_set_vec2(aos, "u_noise_scale", (vec2){noise_scale_x, noise_scale_y});
}

static inline void ssao_update(Renderer* renderer, int w, int h)
{
    ssao_t *ssao = &renderer->ssao;
    // ssao fbo and tex
    glBindFramebuffer(GL_FRAMEBUFFER, ssao->ssao_fbo);
    glBindTexture(GL_TEXTURE_2D, ssao->ssao_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ssao fbo and tex
    glBindFramebuffer(GL_FRAMEBUFFER, ssao->ssao_blur_fbo);
    glBindTexture(GL_TEXTURE_2D, ssao->ssao_blur_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float noise_scale_x = renderer->viewportSize[0] / 4.0f;
    float noise_scale_y = renderer->viewportSize[1] / 4.0f;
    shader_set_vec2(&ssao->ssao_shader, "u_noise_scale", (vec2){noise_scale_x, noise_scale_y});
}

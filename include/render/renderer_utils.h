#pragma once
#include "renderer.h"
#include <math.h>
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
        GL_COLOR_ATTACHMENT3
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

    renderer->settings.shadows_enabled = true;
    renderer->settings.shadows_bias = 0.0001;
    renderer->settings.shadows_spread = 1.7;

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

static inline void cube_init(Renderer *renderer)
{
    float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &renderer->cubeVAO);
        glGenBuffers(1, &renderer->cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, renderer->cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(renderer->cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
}

static inline void cube_render(GLuint cvao)
{
    // render Cube
    glBindVertexArray(cvao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

#include <stb_image.h>
// initializes the active ibl from the renderer
static inline void ibl_init(Renderer *renderer, const char *hdr_name)
{
    ibl_t *ibl = renderer->active_ibl;
    iblShared_t *ibls = &renderer->ibl_shared;
    //ibl_t *ibl = &renderer->ibl;
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    stbi_set_flip_vertically_on_load(true);
    int32_t w, h, nrComponents;
    float *hdrdata;
    uint32_t hdrtex;
    hdrdata = stbi_loadf(hdr_name, &w, &h, &nrComponents, 0);
    if (hdrdata)
    {
        glGenTextures(1, &hdrtex);
        glBindTexture(GL_TEXTURE_2D, hdrtex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, hdrdata); 

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(hdrdata);
    }
    else
    {
        log_error("COULDN'T LOAD HDR IMAGE!");
    } 
    stbi_set_flip_vertically_on_load(false);
#define HDR_SKYBOX_RES 3840
    glGenFramebuffers(1, &renderer->ibl_fbo);
    glGenRenderbuffers(1, &renderer->ibl_rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ibl_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, renderer->ibl_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, HDR_SKYBOX_RES, HDR_SKYBOX_RES);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderer->ibl_rbo);

    glGenTextures(1, &ibl->env_cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->env_cubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        // store each face with 16 bit float values
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, HDR_SKYBOX_RES, HDR_SKYBOX_RES, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    
    mat4 captureProjection;
    glm_perspective(glm_rad(90.0f), 1.0f, 0.1f, 10.0f, captureProjection);
    mat4 captureViews[6];
    glm_lookat(((vec3){0.0f, 0.0f, 0.0f}), ((vec3){1.0f,  0.0f,  0.0f}), ((vec3){0.0f, -1.0f,  0.0f}), captureViews[0]);
    glm_lookat(((vec3){0.0f, 0.0f, 0.0f}), ((vec3){-1.0f,  0.0f,  0.0f}), ((vec3){0.0f, -1.0f,  0.0f}), captureViews[1]);
    glm_lookat(((vec3){0.0f, 0.0f, 0.0f}), ((vec3){0.0f,  1.0f,  0.0f}), ((vec3){0.0f, 0.0f,  1.0f}), captureViews[2]);
    glm_lookat(((vec3){0.0f, 0.0f, 0.0f}), ((vec3){0.0f,  -1.0f,  0.0f}), ((vec3){0.0f, 0.0f,  -1.0f}), captureViews[3]);
    glm_lookat(((vec3){0.0f, 0.0f, 0.0f}), ((vec3){0.0f,  0.0f,  1.0f}), ((vec3){0.0f, -1.0f,  0.0f}), captureViews[4]);
    glm_lookat(((vec3){0.0f, 0.0f, 0.0f}), ((vec3){0.0f,  0.0f,  -1.0f}), ((vec3){0.0f, -1.0f,  0.0f}), captureViews[5]);

    // convert HDR equirectangular environment map to cubemap equivalent
    shader_use(&ibls->hdr_equirec);
    shader_set_int(&ibls->hdr_equirec, "u_equirectangularMap", 0);
    shader_set_mat4(&ibls->hdr_equirec, "u_projection", captureProjection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ibl->env_cubemap);

    glViewport(0, 0, HDR_SKYBOX_RES, HDR_SKYBOX_RES);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ibl_fbo);

    for (unsigned int i = 0; i < 6; ++i)
    {
        shader_set_mat4(&ibls->hdr_equirec, "u_view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ibl->env_cubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cube_render(renderer->cubeVAO); // renders a 1x1 cube
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // irradiance texture
#define IRRADIANCE_RES 32
    glGenTextures(1, &ibl->irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, IRRADIANCE_RES, IRRADIANCE_RES, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // irradiance map generator
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ibl_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, renderer->ibl_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRRADIANCE_RES, IRRADIANCE_RES);

    shader_use(&ibls->irradiance_shader);
    shader_set_int(&ibls->irradiance_shader, "u_environmentMap", 0);
    shader_set_mat4(&ibls->irradiance_shader, "u_projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->env_cubemap);

    glViewport(0, 0, IRRADIANCE_RES, IRRADIANCE_RES); 
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ibl_fbo);
    for (unsigned int i = 0; i < 6; ++i)
    {
        shader_set_mat4(&ibls->irradiance_shader, "u_view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ibl->irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cube_render(renderer->cubeVAO);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // specular map generator
#define SPECULAR_RES 512
    glGenTextures(1, &ibl->prefilter_map);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->prefilter_map);
    for(int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, SPECULAR_RES, SPECULAR_RES, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    shader_use(&ibls->prefilter_shader);
    shader_set_int(&ibls->prefilter_shader, "u_environmentMap", 0);
    shader_set_mat4(&ibls->prefilter_shader, "u_projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->env_cubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ibl_fbo);
    uint32_t maxMipLevels = 5;
    for (uint32_t mip = 0; mip < maxMipLevels; ++mip)
    {
        // resize framebuffer according to mip level size
        uint32_t mipWidth  = SPECULAR_RES * pow(0.5, mip);
        uint32_t mipHeight = SPECULAR_RES * pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, renderer->ibl_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        shader_set_float(&ibls->prefilter_shader, "u_roughness", roughness);
        for (uint32_t i = 0; i < 6; ++i)
        {
            shader_set_mat4(&ibls->prefilter_shader, "u_view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ibl->prefilter_map, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            cube_render(renderer->cubeVAO);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, renderer->viewportSize[0], renderer->viewportSize[1]);
}

void brdf_init(Renderer * renderer)
{
    brdf_t *brdf = &renderer->brdf;
        // brdf LUT
#define LUT_RES 512
    glGenTextures(1, &brdf->brdf_LUT);
    glBindTexture(GL_TEXTURE_2D, brdf->brdf_LUT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, LUT_RES, LUT_RES, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ibl_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, renderer->ibl_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, LUT_RES, LUT_RES);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf->brdf_LUT, 0);

    glViewport(0, 0, LUT_RES, LUT_RES);
    shader_use(&brdf->brdf_shader);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(renderer->quad_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  

    glViewport(0, 0, renderer->viewportSize[0], renderer->viewportSize[1]);
}

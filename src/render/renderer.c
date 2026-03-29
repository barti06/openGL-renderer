#include "renderer.h"
#include "world.h"
#include "ui.h"
#include <log.h>

#define DEFAULT_TONEMAP TONEMAP_ACES
#define DEFAULT_GAMMA 2.2f
#define DEFAULT_BRIGHTNESS 1.0f
#define DEFAULT_EXPOSURE 1.0f

#define DEFAULT_VIGNETTE_STATE 0 // false
#define DEFAULT_VIGNETTE_STRENGTH 2.0f
#define DEFAULT_CA_STATE 0 // false
#define DEFAULT_CA_STRENGTH 2.0f

#define DEFAULT_NEARZ 0.1f
#define DEFAULT_FARZ 100.0f

static inline void gbuffer_setup(Renderer* renderer, int w, int h);

static inline void postFX_setup(Renderer* renderer, int w, int h);

static inline void gbuffer_update(Renderer* renderer, int w, int h);

static inline void postFX_update(Renderer* renderer, int w, int h);

static inline void renderer_model_draw(const Model* model, Renderer* renderer, mat4 world_matrix);

static inline void renderer_get_light(Renderer* renderer, LightComponent* lc, vec3 position);

float quadVertices[] = { 
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
};

void renderer_init(Renderer* renderer, pipeline_t pipeline, int viewportX,
    int viewportY)
{
    // load glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		log_error("\nERROR... engine_init() SAYS: COULDN'T LOAD GLAD!\n");
	}

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    renderer->nearZ = DEFAULT_NEARZ;
    renderer->farZ = DEFAULT_FARZ;
    renderer->viewportSize[0] = viewportX;
    renderer->viewportSize[1] = viewportY;
    // init deferred and forward shaders
	shader_init(&renderer->deferred_shader, "shaders/deferred.vert", "shaders/deferred.frag");
    shader_init(&renderer->forward_shader, "shaders/forward.vert", "shaders/forward.frag");
    // set the active shader
    switch(pipeline)
    {
        case PIPELINE_DEFERRED:
        renderer->active_shader = &renderer->deferred_shader;
        break;
        case PIPELINE_FORWARD:
        renderer->active_shader = &renderer->forward_shader;
        break;
        default:
        break;
    }
    renderer->render_mode = pipeline;

    // generate main quad vao and vbo
    glGenVertexArrays(1, &renderer->quad_VAO);
    glGenBuffers(1, &renderer->quad_VBO);
    glBindVertexArray(renderer->quad_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->quad_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // init shaders
    shader_init(&renderer->light_shader, "shaders/quad.vert", "shaders/lighting.frag");
    shader_init(&renderer->fx_shader, "shaders/quad.vert", "shaders/postfx.frag");
    // bloom shaders
    shader_init(&renderer->bloom_blur_shader, "shaders/quad.vert", "shaders/bloom_blur.frag");

    // init fbos
    gbuffer_setup(renderer, viewportX, viewportY);
    postFX_setup(renderer, viewportX, viewportY);

    // init gbuffer shader vars
    shader_use(&renderer->light_shader);
    shader_set_int(&renderer->light_shader, "g_position", 0);
    shader_set_int(&renderer->light_shader, "g_normal", 1);
    shader_set_int(&renderer->light_shader, "g_albedo", 2);
    shader_set_int(&renderer->light_shader, "g_orm", 3);
    shader_set_int(&renderer->light_shader, "g_emissive", 4);
    shader_set_int(&renderer->light_shader, "g_depth", 5);

    // init post processing shader
    shader_use(&renderer->fx_shader);
    shader_set_int(&renderer->fx_shader, "fx_scene", 0);
    shader_set_int(&renderer->fx_shader, "fx_depth", 1);
    shader_set_int(&renderer->fx_shader, "fx_bloom", 2);
    renderer->gamma = DEFAULT_GAMMA;
    renderer->exposure = DEFAULT_EXPOSURE;
    renderer->brightness = DEFAULT_BRIGHTNESS;
    renderer->tonemap = DEFAULT_TONEMAP;
    renderer->vignette_enabled = DEFAULT_VIGNETTE_STATE;
    renderer->vignette_strength = DEFAULT_VIGNETTE_STRENGTH;
    renderer->CA_enabled = DEFAULT_CA_STATE;
    renderer->CA_strength = DEFAULT_CA_STRENGTH;

    // for gpu timings
    glGenQueries(1, &renderer->geometry_query);
    glGenQueries(1, &renderer->light_query);
    glGenQueries(1, &renderer->fx_query);
    renderer->stats_update_interval = 0.25f; // updates imgui timers four times a second
    renderer->stats_timer = 0.0f;

    renderer->bloom_enabled = true;
    renderer->bloom_threshold = 0.2f;
    renderer->bloom_strength = 1.0f;
    renderer->bloom_blur_passes = 5;
}

void renderer_updates(World* world, Renderer* renderer, int windowX, int windowY)
{
    if(renderer->viewportSize[0] != windowX || renderer->viewportSize[1] != windowY)
    {
        renderer->viewportSize[0] = windowX;
        renderer->viewportSize[1] = windowY;

        if(renderer->render_mode == PIPELINE_DEFERRED)
            gbuffer_update(renderer, renderer->viewportSize[0], renderer->viewportSize[1]);

        postFX_update(renderer, renderer->viewportSize[0], renderer->viewportSize[1]);
    }

    shader_use(renderer->active_shader);
    // send updated camera matrices to geometry shader
    shader_set_mat4(renderer->active_shader, "u_view", world->camera.view);
    shader_set_mat4(renderer->active_shader, "u_projection", world->camera.projection);

    switch(renderer->render_mode)
    {   
        case PIPELINE_DEFERRED:
        shader_use(&renderer->light_shader);
        shader_set_vec3(&renderer->light_shader, "u_camera_position", world->camera.position);
        shader_set_int(&renderer->light_shader, "u_gbuffer_view", (int32_t)renderer->gbuffer_view);
        shader_set_float(&renderer->light_shader, "u_bloom_threshold", renderer->bloom_threshold);
        break;
        case PIPELINE_FORWARD:
        shader_use(&renderer->forward_shader);
        shader_set_vec3(&renderer->forward_shader, "u_camera_position", world->camera.position);
        shader_set_float(&renderer->forward_shader, "u_bloom_threshold", renderer->bloom_threshold);
        default:
        break;
    }

    renderer_postfx_update(renderer);
    // update the camera
    camera_update_matrices(&world->camera, renderer->viewportSize, renderer->nearZ, renderer->farZ);
}

void renderer_draw_world(World* world, Renderer* renderer, double delta_time)
{
    glViewport(0, 0, renderer->viewportSize[0], renderer->viewportSize[1]);
    
    glBeginQuery(GL_TIME_ELAPSED, renderer->geometry_query);
    switch(renderer->render_mode)
    {
        case PIPELINE_DEFERRED:
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->gBuffer_fbo);    // for geometry pass
        break;
        case PIPELINE_FORWARD:
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->fx_fbo);
        break;
        default:
        break;
    }
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for(size_t index = 0; index < world->entities_count; index++)
    {
        uint32_t needed = COMPONENT_TRANSFORM | COMPONENT_RENDERABLE;
        if (!entity_has_component(&world->entities[index], needed))
            continue;

        RenderableComponent* rc = &world->renderables[index];
        if (!renderable_has_flag(rc, RENDER_FLAG_VISIBLE))
            continue;

        if(!renderable_has_flag(rc, RENDER_FLAG_CULL))
            glDisable(GL_CULL_FACE);
        if(renderable_has_flag(rc, RENDER_FLAG_WIREFRAME))
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        if(renderable_has_flag(rc, RENDER_FLAG_BLEND))
            glEnable(GL_BLEND);

        renderer_model_draw(rc->model, renderer, world->transforms[index].world_matrix);

        if (!renderable_has_flag(rc, RENDER_FLAG_CULL))
            glEnable(GL_CULL_FACE);
        if (renderable_has_flag(rc, RENDER_FLAG_WIREFRAME))
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if(renderable_has_flag(rc, RENDER_FLAG_BLEND))
            glDisable(GL_BLEND);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEndQuery(GL_TIME_ELAPSED);

    glBeginQuery(GL_TIME_ELAPSED, renderer->light_query);
    if(renderer->render_mode == PIPELINE_DEFERRED)
    {
        // light pass
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->fx_fbo);
        shader_use(&renderer->light_shader);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        // send pos texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer->g_position);
        // send normal texture
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, renderer->g_normal);
        // send albedo texture
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, renderer->g_albedo);
        // send ormt texture
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, renderer->g_orm);
        // send emissive texture
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, renderer->g_emissive);
        // send depth texture
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, renderer->g_depth);
        // render light pass to a quad
        glBindVertexArray(renderer->quad_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
    }
    glEndQuery(GL_TIME_ELAPSED);

    glBeginQuery(GL_TIME_ELAPSED, renderer->fx_query);
    if(renderer->bloom_enabled)
    {
        // pingpong blur
        bool horizontal = true;
        // first pass reads from bloom_bright, subsequent passes ping-pong between bloom_tex[0/1]
        glActiveTexture(GL_TEXTURE0);
        shader_use(&renderer->bloom_blur_shader);
        shader_set_int(&renderer->bloom_blur_shader, "u_image", 0);
        for(int i = 0; i < renderer->bloom_blur_passes * 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, renderer->bloom_fbo[horizontal]);
            shader_set_bool(&renderer->bloom_blur_shader, "u_horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, i == 0 ? renderer->bloom_bright : renderer->fx_bloom[!horizontal]);
            glBindVertexArray(renderer->quad_VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            horizontal = !horizontal;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // postFX pass
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    shader_use(&renderer->fx_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->fx_scene);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->fx_depth);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderer->bloom_enabled ? renderer->fx_bloom[0] : 0);
    glBindVertexArray(renderer->quad_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
    glEndQuery(GL_TIME_ELAPSED);

    GLuint64 time_geometry, time_light, time_fx;
    glGetQueryObjectui64v(renderer->geometry_query, GL_QUERY_RESULT, &time_geometry);
    glGetQueryObjectui64v(renderer->light_query, GL_QUERY_RESULT, &time_light);
    glGetQueryObjectui64v(renderer->fx_query, GL_QUERY_RESULT, &time_fx);

    float geometry_display = time_geometry / 1000000.0;
    float light_display = time_light / 1000000.0;
    float fx_display = time_fx / 1000000.0;
    renderer->stats_timer += delta_time;
    if (renderer->stats_timer >= renderer->stats_update_interval)
    {
        renderer->stats_fps = 1.0 / delta_time;
        renderer->stats_geometry_ms = geometry_display;
        renderer->stats_light_ms = light_display;
        renderer->stats_fx_ms = fx_display;
        renderer->stats_timer = 0.0f;
    }
}

void renderer_gbuffer_reload(Renderer* renderer)
{
    shader_reload_frag(&renderer->light_shader);
    shader_use(&renderer->light_shader);
    shader_set_int(&renderer->light_shader, "g_position", 0);
    shader_set_int(&renderer->light_shader, "g_normal", 1);
    shader_set_int(&renderer->light_shader, "g_albedo", 2);
    shader_set_int(&renderer->light_shader, "g_orm", 3);
    shader_set_int(&renderer->light_shader, "g_emissive", 4);
    shader_set_int(&renderer->light_shader, "g_depth", 5);
}
void renderer_gbuffer_update(Renderer* renderer, vec3 camera_pos)
{
    shader_use(&renderer->light_shader);
    // this function is not too long and probably should just say this directly on renderer update but wtv
    shader_set_vec3(&renderer->light_shader, "u_camera_position", camera_pos);
    shader_set_int(&renderer->light_shader, "u_gbuffer_view", (int32_t)renderer->gbuffer_view);
}

void renderer_postfx_reload(Renderer* renderer)
{
    shader_reload_frag(&renderer->fx_shader);
    shader_use(&renderer->fx_shader);
    shader_set_int(&renderer->fx_shader, "fx_scene", 0);
    shader_set_int(&renderer->fx_shader, "fx_depth", 1);
    shader_set_int(&renderer->fx_shader, "fx_bloom", 2);
}

void renderer_postfx_update(Renderer* renderer)
{
    // tonemappig updates
    shader_use(&renderer->fx_shader);
    shader_set_int(&renderer->fx_shader, "u_tonemap", (int32_t)renderer->tonemap);
    shader_set_float(&renderer->fx_shader, "u_gamma", renderer->gamma);
    shader_set_float(&renderer->fx_shader, "u_exposure", renderer->exposure);
    shader_set_float(&renderer->fx_shader, "u_brightness", renderer->brightness);

    // disable any post processing when checking specific gBuffer renders
    bool should_disable = (renderer->gbuffer_view > 0);
    shader_set_bool(&renderer->fx_shader, "u_gbuffer_viewing", should_disable);

    // send other post processing settings
    shader_set_bool(&renderer->fx_shader, "u_vignette_enabled", renderer->vignette_enabled);
    shader_set_float(&renderer->fx_shader, "u_vignette_strength", renderer->vignette_strength);
    shader_set_bool(&renderer->fx_shader, "u_chromatic_aberration_enabled", renderer->CA_enabled);
    shader_set_float(&renderer->fx_shader, "u_chromatic_aberration_strength", renderer->CA_strength);
    shader_set_bool(&renderer->fx_shader, "u_bloom_enabled", renderer->bloom_enabled);
    shader_set_float(&renderer->fx_shader, "u_bloom_strength", renderer->bloom_strength);
}

const char* gbuffer_options[] = {
    "Final",
    "Position",
    "Normal",
    "Albedo",
    "Occlusion",
    "Roughness",
    "Metalness",
    "Emissive",
    "Depth"
};

const char* tonemap_options[] = {
    "ACES",
    "Reinhard",
    "Filmic"
};

const char* rendering_pipelines[] = {
    "Forward",
    "Deferred"
};

void renderer_ui(Renderer* renderer)
{
    igBegin("Renderer", NULL, 0);
    igText("FPS: %.3f", renderer->stats_fps);
    igText("Geometry pass: %.3f ms", renderer->stats_geometry_ms);
    igText("Lighting pass: %.3f ms", renderer->stats_light_ms);
    igText("Post-Processing pass: %.3f ms", renderer->stats_fx_ms);
    igSeparator();

    bool pipeline_open = igCollapsingHeader_TreeNodeFlags("Pipeline", 0);
    if(pipeline_open)
    {
        if(renderer->render_mode == PIPELINE_DEFERRED)
            igCombo_Str_arr("GBuffer view", &renderer->gbuffer_view, gbuffer_options, GBUFFER_MAX, -1);
        igSeparator();
    }

    bool settings_open = igCollapsingHeader_TreeNodeFlags("Renderer settings", 0);
    if(settings_open)
    {
        igCombo_Str_arr("Tone mapper", &renderer->tonemap, tonemap_options, TONEMAP_MAX, -1);
        igSliderFloat("Gamma", &renderer->gamma, 0.0f, 3.0f, "%.1f", 0);
        igSliderFloat("Exposure", &renderer->exposure, 0.0f, 2.0f, "%.1f", 0);
        igSliderFloat("Brightness", &renderer->brightness, 0.0f, 2.0f, "%.1f", 0);
        igCheckbox("Vignette", &renderer->vignette_enabled);
        if(renderer->vignette_enabled)
            igSliderFloat("Vignette strength", &renderer->vignette_strength, 0.0f, 2.0f, "%.1f", 0);
        igCheckbox("Chromatic aberration", &renderer->CA_enabled);
        if(renderer->CA_enabled)
            igSliderFloat("Chromatic aberration strength", &renderer->CA_strength, 0.0f, 4.0f, "%.1f", 0);
        igCheckbox("Bloom", &renderer->bloom_enabled);
        if(renderer->bloom_enabled)
        {
            igSliderFloat("Bloom strength", &renderer->bloom_strength, 0.0f, 50.0f, "%.1f", 0);
            igSliderFloat("Bloom threshold", &renderer->bloom_threshold, 0.0f, 50.0f, "%.1f", 0);
            igSliderInt("Bloom blur passes", &renderer->bloom_blur_passes, 0, 250, "%d", 0);
        }
    }
    igEnd();
}

static inline void gbuffer_setup(Renderer* renderer, int w, int h)
{
    glGenFramebuffers(1, &renderer->gBuffer_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->gBuffer_fbo);

    // main pass position texture
    glGenTextures(1, &renderer->g_position);
    glBindTexture(GL_TEXTURE_2D, renderer->g_position);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->g_position, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass normal texture
    glGenTextures(1, &renderer->g_normal);
    glBindTexture(GL_TEXTURE_2D, renderer->g_normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, renderer->g_normal, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass albedo texture
    glGenTextures(1, &renderer->g_albedo);
    glBindTexture(GL_TEXTURE_2D, renderer->g_albedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, renderer->g_albedo, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass orm (occlusion, roughness & metalness) texture
    glGenTextures(1, &renderer->g_orm);
    glBindTexture(GL_TEXTURE_2D, renderer->g_orm);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, renderer->g_orm, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass emissive texture
    glGenTextures(1, &renderer->g_emissive);
    glBindTexture(GL_TEXTURE_2D, renderer->g_emissive);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, renderer->g_emissive, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass depth texture
    glGenTextures(1, &renderer->g_depth);
    glBindTexture(GL_TEXTURE_2D, renderer->g_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderer->g_depth, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint attachments[] = { 
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, 
        GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 
    };
    glDrawBuffers((sizeof(attachments) / sizeof(GLuint)), attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	    log_error("ERROR... renderer_init() says: FRAMEBUFFER INCOMPLETE.\n");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static inline void postFX_setup(Renderer* renderer, int w, int h)
{
    glGenFramebuffers(1, &renderer->fx_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fx_fbo);

    // all of gbuffer's lighting calculations
    glGenTextures(1, &renderer->fx_scene);
    glBindTexture(GL_TEXTURE_2D, renderer->fx_scene);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->fx_scene, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // used in forward rendering
    glGenTextures(1, &renderer->fx_depth);
    glBindTexture(GL_TEXTURE_2D, renderer->fx_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderer->fx_depth, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // bloom brightness extractor
    glGenTextures(1, &renderer->bloom_bright);
    glBindTexture(GL_TEXTURE_2D, renderer->bloom_bright);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, renderer->bloom_bright, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint attachments[] = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1
    };
    glDrawBuffers(sizeof(attachments) / sizeof(GLuint), attachments);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong blur fbos
    glGenFramebuffers(2, renderer->bloom_fbo);
    glGenTextures(2, renderer->fx_bloom);
    for(int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->bloom_fbo[i]);
        glBindTexture(GL_TEXTURE_2D, renderer->fx_bloom[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->fx_bloom[i], 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

static inline void gbuffer_update(Renderer* renderer, int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->gBuffer_fbo);

    // main pass position texture
    glBindTexture(GL_TEXTURE_2D, renderer->g_position);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass normal texture
    glBindTexture(GL_TEXTURE_2D, renderer->g_normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass albedo texture
    glBindTexture(GL_TEXTURE_2D, renderer->g_albedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass orm (occlusion, roughness & metalness) texture
    glBindTexture(GL_TEXTURE_2D, renderer->g_orm);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass emissive textures
    glBindTexture(GL_TEXTURE_2D, renderer->g_emissive);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass depth texture
    glBindTexture(GL_TEXTURE_2D, renderer->g_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static inline void postFX_update(Renderer* renderer, int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fx_fbo);
    // all of gbuffer's lighting calculations
    glBindTexture(GL_TEXTURE_2D, renderer->fx_scene);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // used in forward
    glBindTexture(GL_TEXTURE_2D, renderer->fx_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // bloom brightness update
    glBindTexture(GL_TEXTURE_2D, renderer->bloom_bright);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // bloom blur update
    for(int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->bloom_fbo[i]);
        glBindTexture(GL_TEXTURE_2D, renderer->fx_bloom[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

static inline void renderer_model_draw(const Model* model, Renderer* renderer, mat4 world_matrix)
{
    if (!model || !model->is_loaded || !model->meshes)
        return;

    Shader* shader = renderer->active_shader;

    // activate the shader
    shader_use(shader);

    // draw each primitive from a model
    for (uint32_t mi = 0; mi < model->mesh_count; mi++)
    {
        const Mesh* mesh = &model->meshes[mi];

        // multiply each model's mesh matrix by the world matrix of the entity that owns them
        mat4 final_transform;
        // avx makes EVERYTHING explode and i am too lazy to be bothered :D
        glm_mat4_mul_sse2(world_matrix, (vec4*)mesh->transform, final_transform);

        shader_set_mat4(shader, "u_model", final_transform);
        
        for(uint32_t pi = 0; pi < mesh->primitive_count; pi++)
        {
            const Primitive* current_primitive = &mesh->primitives[pi];
            
            bool has_albedo = false;
            bool has_metallic_roughness = false;
            bool has_normal = false;
            bool has_emissive = false;
            bool has_ao = false;

            if (current_primitive->material.pbr.albedo)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.pbr.albedo);
                shader_set_int(shader, "u_albedo", 0);
                has_albedo = true;
            }
            if(current_primitive->material.pbr.metallic_roughness)
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.pbr.metallic_roughness);
                shader_set_int(shader, "u_metallic_roughness", 1);
                has_metallic_roughness = true;
            }
            if(current_primitive->material.shared.normal)
            {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.shared.normal);
                shader_set_int(shader, "u_normal", 2);
                shader_set_vec2(shader, "u_normal_scale", current_primitive->material.shared.normal_scale);
                has_normal = true;
            }
            if(current_primitive->material.shared.emissive)
            {
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.shared.emissive);
                shader_set_int(shader, "u_emissive", 3);
                shader_set_bool(shader, "u_has_emissive_texcoord", current_primitive->material.shared.has_emissive_texcoord);
                has_emissive = true;
            }
            if(current_primitive->material.shared.ambient_occlusion)
            {
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.shared.ambient_occlusion);
                shader_set_int(shader, "u_ao", 4);
                shader_set_float(shader, "u_occlusion_strength", current_primitive->material.shared.occlusion_strength);
                shader_set_vec2(shader, "u_occlusion_scale", current_primitive->material.shared.occlusion_scale);
                shader_set_bool(shader, "u_has_occlusion_texcoord", current_primitive->material.shared.has_occlusion_texcoord);
                has_ao = true;
            }
            if(current_primitive->material.has_iridescence)
            {
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.iridescence.texture);
                shader_set_int(shader, "u_iridescence", 5);
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.iridescence.thickness_texture);
                shader_set_int(shader, "u_iridescence_thickness", 6);
                
                shader_set_float(shader, "u_iridescence_thickness_max", current_primitive->material.iridescence.thickness_max);
                shader_set_float(shader, "u_iridescence_thickness_min", current_primitive->material.iridescence.thickness_min);
                shader_set_float(shader, "u_iridescence_factor", current_primitive->material.iridescence.factor);
                shader_set_float(shader, "u_iridescence_ior", current_primitive->material.iridescence.ior);
            }
            if(current_primitive->material.has_volume)
            {
                glActiveTexture(GL_TEXTURE7);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.volume.thickness_texture);
                shader_set_int(shader, "u_volume_thickness", 7);
                shader_set_float(shader, "u_volume_thickness_factor", current_primitive->material.volume.thickness_factor);
                shader_set_float(shader, "u_volume_attenuation_distance", current_primitive->material.volume.attenuation_distance);
                shader_set_vec3(shader, "u_volume_attenuation_color", current_primitive->material.volume.attenuation_color);
            }

            shader_set_bool(shader, "u_has_albedo", has_albedo);
            shader_set_vec4(shader, "u_albedo_factor", current_primitive->material.pbr.albedo_factor);

            shader_set_bool(shader, "u_has_metallic_roughness", has_metallic_roughness);
            shader_set_float(shader, "u_metallic_factor", current_primitive->material.pbr.metallic_factor);
            shader_set_float(shader, "u_roughness_factor", current_primitive->material.pbr.roughness_factor);

            shader_set_bool(shader, "u_has_normal", has_normal);

            shader_set_bool(shader, "u_has_emissive", has_emissive);
            shader_set_float(shader, "u_emissive_strength", current_primitive->material.shared.emissive_strength);
            shader_set_vec3(shader, "u_emissive_factor", current_primitive->material.shared.emissive_factor);

            shader_set_bool(shader, "u_has_ao", has_ao);

            shader_set_bool(shader, "u_has_iridescence", current_primitive->material.has_iridescence);
            shader_set_bool(shader, "u_has_volume", current_primitive->material.has_volume);
            shader_set_bool(shader, "u_unlit", current_primitive->material.unlit);

            glActiveTexture(GL_TEXTURE0);

            glBindVertexArray(current_primitive->VAO);
            glDrawElements(GL_TRIANGLES, current_primitive->index_count, current_primitive->index_type, (void*)0);
            glBindVertexArray(0);
        }
    }
}

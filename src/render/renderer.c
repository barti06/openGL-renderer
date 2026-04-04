#include "world.h"
#include "ui.h"
#include "renderer_utils.h"

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

static inline void renderer_model_draw(const Model* model, Renderer* renderer, mat4 world_matrix, Camera *camera);

static inline void renderer_model_simpleDraw(const Model* model, Shader *shader, mat4 world_matrix);

float quadVertices[] = { 
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
};

void renderer_init(Renderer* renderer, int viewportX,
    int viewportY)
{
    // load glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		log_error("\nERROR... engine_init() SAYS: COULDN'T LOAD GLAD!\n");
	}

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LEQUAL); // for skybox

    renderer->nearZ = DEFAULT_NEARZ;
    renderer->farZ = DEFAULT_FARZ;

    renderer->viewportSize[0] = viewportX;
    renderer->viewportSize[1] = viewportY;

    // init deferred and forward shaders
	shader_init(&renderer->gbuffer.deferred_shader, "shaders/geometry.vert", "shaders/deferred.frag");
    shader_init(&renderer->shadowMap_shader, "shaders/geometry_shadowMap.vert", "shaders/shadowMap.frag");

    renderer->active_shader = &renderer->gbuffer.deferred_shader;


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
    shader_init(&renderer->gbuffer.light_shader, "shaders/quad.vert", "shaders/lighting.frag");
    shader_init(&renderer->fx.fx_shader, "shaders/quad.vert", "shaders/postfx.frag");
    // bloom shaders
    shader_init(&renderer->bloom.bloom_blur_shader, "shaders/quad.vert", "shaders/bloom_blur.frag");
    //ssao shaders
    shader_init(&renderer->ssao.ssao_shader, "shaders/quad.vert", "shaders/hbao.frag");
    shader_init(&renderer->ssao.ssao_blur_shader, "shaders/quad.vert", "shaders/ssao_blur.frag");
    
    ibl_t *ibl = &renderer->ibl;
    // skybox shaders
    shader_init(&ibl->hdr_equirec, "shaders/hdr_equirec.vert", "shaders/hdr_equirec.frag");
    shader_init(&ibl->hdr_bg, "shaders/hdr_bg.vert", "shaders/hdr_bg.frag");
    shader_use(&ibl->hdr_bg);
    shader_set_int(&ibl->hdr_bg, "u_environmentMap", 0); // tell ogl skybox tex is sent at slot 0
    shader_init(&ibl->irradiance_shader, "shaders/hdr_equirec.vert", "shaders/hdr_irr.frag");

    // init fbos
    gbuffer_setup(renderer, viewportX, viewportY);
    postFX_setup(renderer, viewportX, viewportY);
    ssao_setup(renderer, viewportX, viewportY);

    Shader *fs = &renderer->fx.fx_shader;

    // init post processing shader
    shader_use(fs);
    shader_set_int(fs, "fx_scene", 0);
    shader_set_int(fs, "fx_depth", 1);
    shader_set_int(fs, "fx_bloom", 2);

    // setup render settings (ao sets itself when calling ssao_setup())
    renderSettings_t *rs = &renderer->settings;
    rs->gamma = DEFAULT_GAMMA;
    rs->exposure = DEFAULT_EXPOSURE;
    rs->brightness = DEFAULT_BRIGHTNESS;
    rs->tonemap = DEFAULT_TONEMAP;
    rs->vignette_enabled = DEFAULT_VIGNETTE_STATE;
    rs->vignette_strength = DEFAULT_VIGNETTE_STRENGTH;
    rs->CA_enabled = DEFAULT_CA_STATE;
    rs->CA_strength = DEFAULT_CA_STRENGTH;
    rs->bloom_enabled = true;
    rs->bloom_threshold = 0.2f;
    rs->bloom_strength = 1.0f;
    rs->bloom_blur_passes = 5;

    // for gpu timings
    renderTimers_t *rt = &renderer->timers;
    glGenQueries(1, &rt->geometry_query);
    glGenQueries(1, &rt->light_query);
    glGenQueries(1, &rt->fx_query);
    glGenQueries(1, &rt->ssao_query);
    glGenQueries(1, &rt->ssao_blur_query);
    glGenQueries(1, &rt->shadow_query);
    rt->stats_update_interval = 0.25f; // updates imgui timers four times a second
    rt->stats_timer = 0.0f;

    shadowMap_init(&renderer->shadow);

    cube_init(renderer);

    ibl_init(renderer, "cloudy.hdr");
}

void renderer_updates(World* world, Renderer* renderer, int windowX, int windowY)
{
    if(renderer->viewportSize[0] != windowX || renderer->viewportSize[1] != windowY)
    {
        renderer->viewportSize[0] = windowX;
        renderer->viewportSize[1] = windowY;

        gbuffer_update(renderer, renderer->viewportSize[0], renderer->viewportSize[1]);

        ssao_update(renderer, renderer->viewportSize[0], renderer->viewportSize[1]);
        postFX_update(renderer, renderer->viewportSize[0], renderer->viewportSize[1]);
    }

    shader_use(renderer->active_shader);
    // send updated camera matrices to geometry shader
    shader_set_mat4(renderer->active_shader, "u_view", world->camera.view);
    shader_set_mat4(renderer->active_shader, "u_projection", world->camera.projection);

    ibl_t *ibl = &renderer->ibl;
    shader_use(&ibl->hdr_bg);
    shader_set_mat4(&ibl->hdr_bg, "u_projection", world->camera.projection);
    shader_set_mat4(&ibl->hdr_bg, "u_view", world->camera.view);

    renderSettings_t *rs = &renderer->settings;
    shader_use(&renderer->gbuffer.light_shader);
    shader_set_mat4(&renderer->gbuffer.light_shader, "u_inv_viewproj", world->camera.inv_viewproj);
    shader_set_vec3(&renderer->gbuffer.light_shader, "u_camera_position", world->camera.position);
    shader_set_int(&renderer->gbuffer.light_shader, "u_gbuffer_view", rs->gbuffer_view);
    shader_set_float(&renderer->gbuffer.light_shader, "u_bloom_threshold", rs->bloom_threshold);
    shader_set_bool(&renderer->gbuffer.light_shader, "u_ssao_enabled", rs->ssao_enabled);
    shader_set_bool(&renderer->gbuffer.light_shader, "u_shadows_enabled", renderer->settings.shadows_enabled);
    shader_set_float(&renderer->gbuffer.light_shader, "u_shadow_bias", renderer->settings.shadows_bias);
    shader_set_float(&renderer->gbuffer.light_shader, "u_shadow_spread", renderer->settings.shadows_spread);

    ssao_t *ssao = &renderer->ssao;
    shader_use(&ssao->ssao_shader);
    shader_set_mat4(&ssao->ssao_shader, "u_view", world->camera.view);
    shader_set_mat4(&ssao->ssao_shader, "u_projection", world->camera.projection);
    shader_set_mat4(&ssao->ssao_shader, "u_inv_proj", world->camera.inv_proj);
    shader_set_float(&ssao->ssao_shader, "u_radius", rs->ssao_radius);
    shader_set_float(&ssao->ssao_shader, "u_bias", rs->ssao_bias);
    shader_set_float(&ssao->ssao_shader, "u_strength", rs->ssao_strength);
    shader_set_int(&ssao->ssao_shader, "u_directions", rs->hbao_directions);
    shader_set_int(&ssao->ssao_shader, "u_steps", rs->hbao_steps);

    renderer_postfx_update(renderer);
    // update the camera
    camera_update_matrices(&world->camera, renderer->viewportSize, renderer->nearZ, renderer->farZ);
}

void renderer_draw_world(World* world, Renderer* renderer, double delta_time)
{

    // shadow pass
    glBeginQuery(GL_TIME_ELAPSED, renderer->timers.shadow_query);
    if(world->update_shadow)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->shadow.fbo);
        shadowMap_pass(&renderer->shadowMap_shader, &renderer->shadow, world->dirlight_dir);
        for(size_t index = 0; index < world->entities_count; index++)
        {
            if (!entity_has_component(&world->entities[index], COMPONENT_RENDERABLE))
                continue;

            RenderableComponent* rc = &world->renderables[index];
            if (!renderable_has_flag(rc, RENDER_FLAG_VISIBLE))
                continue;

            glDisable(GL_CULL_FACE);
            renderer_model_simpleDraw(rc->model, &renderer->shadowMap_shader, world->transforms[index].world_matrix);
            glEnable(GL_CULL_FACE);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        world->update_shadow = false;
    }
    glEndQuery(GL_TIME_ELAPSED);

    // gpass
    glViewport(0, 0, renderer->viewportSize[0], renderer->viewportSize[1]);
    glBeginQuery(GL_TIME_ELAPSED, renderer->timers.geometry_query);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->gbuffer.gBuffer_fbo);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for(size_t index = 0; index < world->entities_count; index++)
    {
        if (!entity_has_component(&world->entities[index], COMPONENT_RENDERABLE))
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

        renderer_model_draw(rc->model, renderer, world->transforms[index].world_matrix, &world->camera);

        if (!renderable_has_flag(rc, RENDER_FLAG_CULL))
            glEnable(GL_CULL_FACE);
        if (renderable_has_flag(rc, RENDER_FLAG_WIREFRAME))
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if(renderable_has_flag(rc, RENDER_FLAG_BLEND))
            glDisable(GL_BLEND);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEndQuery(GL_TIME_ELAPSED);

    ssao_t *ssao = &renderer->ssao;
    // ssao pass
    glBeginQuery(GL_TIME_ELAPSED, renderer->timers.ssao_query);
    glBindFramebuffer(GL_FRAMEBUFFER, ssao->ssao_fbo);
    glClear(GL_COLOR_BUFFER_BIT);
    shader_use(&ssao->ssao_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_normal);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_depth);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssao->ssao_noise_texture);
    glBindVertexArray(renderer->quad_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEndQuery(GL_TIME_ELAPSED);

    // ssao blur pass
    glBeginQuery(GL_TIME_ELAPSED, renderer->timers.ssao_blur_query);
    glBindFramebuffer(GL_FRAMEBUFFER, ssao->ssao_blur_fbo);
    glClear(GL_COLOR_BUFFER_BIT);
    shader_use(&ssao->ssao_blur_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssao->ssao_texture);
    glBindVertexArray(renderer->quad_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEndQuery(GL_TIME_ELAPSED);

    postEffects_t *fx = &renderer->fx;
    ibl_t *ibl = &renderer->ibl;
    // light pass
    glBeginQuery(GL_TIME_ELAPSED, renderer->timers.light_query);
    glBindFramebuffer(GL_FRAMEBUFFER, fx->fx_fbo);
    shader_use(&renderer->gbuffer.light_shader);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    // send normal texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_normal);
    // send albedo texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_albedo);
    // send ormt texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_orm);
    // send emissive texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_emissive);
    // send depth texture
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, renderer->gbuffer.g_depth);
    // send ssao texture
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, ssao->ssao_blur_texture);
    // send shadow
    shadowMap_send(&renderer->gbuffer.light_shader, 6, &renderer->shadow);
    // send irradiance
    glActiveTexture(GL_TEXTURE7);
    shader_set_int(&renderer->gbuffer.light_shader, "u_irradiance_map", 7);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->irradianceMap);

    // render light pass to a quad
    glBindVertexArray(renderer->quad_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // skybox pass
    glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->gbuffer.gBuffer_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fx->fx_fbo);
    glBlitFramebuffer(0, 0, renderer->viewportSize[0], renderer->viewportSize[1], 0, 0, renderer->viewportSize[0], renderer->viewportSize[1], GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, fx->fx_fbo);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    shader_use(&ibl->hdr_bg);
    glActiveTexture(GL_TEXTURE0);
    shader_set_int(&ibl->hdr_bg, "u_environmentMap", 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->env_cubemap);
    glDisable(GL_CULL_FACE);
    cube_render(renderer->cubeVAO);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEndQuery(GL_TIME_ELAPSED);

    glBeginQuery(GL_TIME_ELAPSED, renderer->timers.fx_query);
    // bloom pass
    renderSettings_t *rs = &renderer->settings;
    bloom_t *b = &renderer->bloom;
    if(rs->bloom_enabled)
    {
        // pingpong blur
        bool horizontal = true;
        // first pass reads from bloom_bright then other passes pingpong between bloom_tex
        glActiveTexture(GL_TEXTURE0);
        shader_use(&b->bloom_blur_shader);
        shader_set_int(&b->bloom_blur_shader, "u_image", 0);
        for(int i = 0; i < rs->bloom_blur_passes * 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, b->bloom_fbo[horizontal]);
            shader_set_bool(&b->bloom_blur_shader, "u_horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, i == 0 ? b->bloom_bright : b->fx_bloom[!horizontal]);
            glBindVertexArray(renderer->quad_VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            horizontal = !horizontal;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    // postFX pass
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    shader_use(&fx->fx_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fx->fx_scene);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fx->fx_depth);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, b->fx_bloom[0]);
    glBindVertexArray(renderer->quad_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
    glEndQuery(GL_TIME_ELAPSED);

    renderTimers_t *rt = &renderer->timers;

    GLuint64 time_geometry, time_light, time_fx, time_ssao, time_ssao_blur, time_shadows;
    glGetQueryObjectui64v(rt->geometry_query, GL_QUERY_RESULT, &time_geometry);
    glGetQueryObjectui64v(rt->light_query, GL_QUERY_RESULT, &time_light);
    glGetQueryObjectui64v(rt->fx_query, GL_QUERY_RESULT, &time_fx);
    glGetQueryObjectui64v(rt->ssao_query, GL_QUERY_RESULT, &time_ssao);
    glGetQueryObjectui64v(rt->ssao_blur_query, GL_QUERY_RESULT, &time_ssao_blur);
    glGetQueryObjectui64v(rt->shadow_query, GL_QUERY_RESULT, &time_shadows);

#define MILISECONDS 1000000.0
    float geometry_display = time_geometry / MILISECONDS;
    float light_display = time_light / MILISECONDS;
    float fx_display = time_fx / MILISECONDS;
    float ssao_display = time_ssao / MILISECONDS;
    float ssao_blur_display = time_ssao_blur / MILISECONDS;
    float shadow_display = time_shadows / MILISECONDS;
    rt->stats_timer += delta_time;
    if (rt->stats_timer >= rt->stats_update_interval)
    {
        rt->stats_fps = 1.0 / delta_time;
        rt->stats_geometry_ms = geometry_display;
        rt->stats_light_ms = light_display;
        rt->stats_fx_ms = fx_display;
        rt->stats_ssao_ms = ssao_display;
        rt->stats_ssao_blur_ms = ssao_blur_display;
        rt->stats_shadows_ms = shadow_display;
        rt->stats_timer = 0.0f;
    }
}

void renderer_gbuffer_reload(Renderer* renderer)
{
    Shader *ds = &renderer->gbuffer.deferred_shader;
    Shader *ls = &renderer->gbuffer.light_shader;
    shader_reload_frag(ls);
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

void renderer_postfx_reload(Renderer* renderer)
{
    postEffects_t *fx = &renderer->fx;
    shader_reload_frag(&fx->fx_shader);
    shader_use(&fx->fx_shader);
    shader_set_int(&fx->fx_shader, "fx_scene", 0);
    shader_set_int(&fx->fx_shader, "fx_depth", 1);
    shader_set_int(&fx->fx_shader, "fx_bloom", 2);
}

void renderer_postfx_update(Renderer* renderer)
{
    renderSettings_t *rs = &renderer->settings;
    Shader *fxs = &renderer->fx.fx_shader;
    // tonemappig updates
    shader_use(fxs);
    shader_set_int(fxs, "u_tonemap", rs->tonemap);
    shader_set_float(fxs, "u_gamma", rs->gamma);
    shader_set_float(fxs, "u_exposure", rs->exposure);
    shader_set_float(fxs, "u_brightness", rs->brightness);

    // disable any post processing when checking specific gBuffer renders
    bool should_disable = (rs->gbuffer_view > 0);
    shader_set_bool(fxs, "u_gbuffer_viewing", should_disable);

    // send other post processing settings
    shader_set_bool(fxs, "u_vignette_enabled", rs->vignette_enabled);
    shader_set_float(fxs, "u_vignette_strength", rs->vignette_strength);
    shader_set_bool(fxs, "u_chromatic_aberration_enabled", rs->CA_enabled);
    shader_set_float(fxs, "u_chromatic_aberration_strength", rs->CA_strength);
    shader_set_bool(fxs, "u_bloom_enabled", rs->bloom_enabled);
    shader_set_float(fxs, "u_bloom_strength", rs->bloom_strength);
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
    "Depth",
    "Unblured bloom",
    "DirLight shadow"
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
    igText("FPS: %.3f", renderer->timers.stats_fps);
    igText("Geometry pass: %.3f ms", renderer->timers.stats_geometry_ms);
    igText("Lighting pass: %.3f ms", renderer->timers.stats_light_ms);
    igText("SSAO pass: %.3f ms", renderer->timers.stats_ssao_ms);
    igText("SSAO blur pass: %.3f ms", renderer->timers.stats_ssao_blur_ms);
    igText("Post-Processing pass: %.3f ms", renderer->timers.stats_fx_ms);
    igText("Shadows pass: %.3f", renderer->timers.stats_shadows_ms);
    igSeparator();

    renderSettings_t *rs = &renderer->settings;

    bool pipeline_open = igCollapsingHeader_TreeNodeFlags("Pipeline", 0);
    if(pipeline_open)
    {
        igCombo_Str_arr("GBuffer view", &rs->gbuffer_view, gbuffer_options, GBUFFER_MAX, -1);
        igSeparator();
    }

    bool settings_open = igCollapsingHeader_TreeNodeFlags("Renderer settings", 0);
    if(settings_open)
    {
        igCombo_Str_arr("Tone mapper", &rs->tonemap, tonemap_options, TONEMAP_MAX, -1);
        igSliderFloat("Gamma", &rs->gamma, 0.0f, 3.0f, "%.1f", 0);
        igSliderFloat("Exposure", &rs->exposure, 0.0f, 2.0f, "%.1f", 0);
        igSliderFloat("Brightness", &rs->brightness, 0.0f, 2.0f, "%.1f", 0);
        igCheckbox("Vignette", &rs->vignette_enabled);
        if(rs->vignette_enabled)
            igSliderFloat("Vignette strength", &rs->vignette_strength, 0.0f, 2.0f, "%.1f", 0);
        igCheckbox("Chromatic aberration", &rs->CA_enabled);
        if(rs->CA_enabled)
            igSliderFloat("Chromatic aberration strength", &rs->CA_strength, 0.0f, 4.0f, "%.1f", 0);
        igCheckbox("Bloom", &rs->bloom_enabled);
        if(rs->bloom_enabled)
        {
            igSliderFloat("Bloom strength", &rs->bloom_strength, 0.0f, 20.0f, "%.1f", 0);
            igSliderFloat("Bloom threshold", &rs->bloom_threshold, 0.0f, 1.0f, "%.2f", 0);
            igSliderInt("Bloom blur passes", &rs->bloom_blur_passes, 0, 100, "%d", 0);
        }
        igCheckbox("SSAO", &rs->ssao_enabled);
        if(rs->ssao_enabled)
        {
            igSliderFloat("SSAO strength", &rs->ssao_strength, 0.0f, 20.0f, "%.1f", 0);
            igSliderFloat("SSAO bias", &rs->ssao_bias, 0.0f, 1.0f, "%.3f", 0);
            igSliderFloat("SSAO radius", &rs->ssao_radius, 0.0f, 10.0f, "%.1f", 0);
            igSliderInt("SSAO directions", &rs->hbao_directions, 0, 50, "%d", 0);
            igSliderInt("SSAO steps", &rs->hbao_steps, 0, 50, "%d", 0);
        }
        igCheckbox("Shadows", &rs->shadows_enabled);
        if(rs->shadows_enabled)
        {
            igSliderFloat("Shadow bias", &rs->shadows_bias, 0.0f, 0.025f, "%.4f", 0);
            igSliderFloat("Shadow spread", &rs->shadows_spread, 0.5f, 3.0f, "%.1f", 0);
        }
    }
    igEnd();
}


static inline void renderer_model_draw(const Model* model, Renderer* renderer, mat4 world_matrix, Camera *camera)
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
        
        vec3 m_aabb[2];
        glm_aabb_transform(mesh->aabb, world_matrix, m_aabb);
        if(!glm_aabb_frustum(m_aabb, camera->frustum))
            continue;
        
        // multiply each model's mesh matrix by the world matrix of the entity that owns them
        mat4 final_transform;
        // avx makes EVERYTHING explode and i am too lazy to be bothered to fix it :D
        glm_mat4_mul_sse2(world_matrix, (vec4*)mesh->transform, final_transform);
        shader_set_mat4(shader, "u_model", final_transform);
        
        for(uint32_t pi = 0; pi < mesh->primitive_count; pi++)
        {
            const Primitive* current_primitive = &mesh->primitives[pi];
            if(current_primitive->material.has_transmission)
                continue;

            bool has_albedo = false;
            bool has_metallic_roughness = false;
            bool has_normal = false;
            bool has_emissive = false;
            bool has_ao = false;

            if (current_primitive->material.pbr.albedo)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.pbr.albedo);
                has_albedo = true;
            }
            if(current_primitive->material.pbr.metallic_roughness)
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.pbr.metallic_roughness);
                has_metallic_roughness = true;
            }
            if(current_primitive->material.shared.normal)
            {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.shared.normal);
                shader_set_vec2(shader, "u_normal_scale", current_primitive->material.shared.normal_scale);
                has_normal = true;
            }
            if(current_primitive->material.shared.emissive)
            {
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.shared.emissive);
                shader_set_bool(shader, "u_has_emissive_texcoord", current_primitive->material.shared.has_emissive_texcoord);
                has_emissive = true;
            }
            if(current_primitive->material.shared.ambient_occlusion)
            {
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.shared.ambient_occlusion);
                shader_set_float(shader, "u_occlusion_strength", current_primitive->material.shared.occlusion_strength);
                shader_set_vec2(shader, "u_occlusion_scale", current_primitive->material.shared.occlusion_scale);
                shader_set_bool(shader, "u_has_occlusion_texcoord", current_primitive->material.shared.has_occlusion_texcoord);
                has_ao = true;
            }
            if(current_primitive->material.has_iridescence)
            {
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.iridescence.texture);
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.iridescence.thickness_texture);
                
                shader_set_float(shader, "u_iridescence_thickness_max", current_primitive->material.iridescence.thickness_max);
                shader_set_float(shader, "u_iridescence_thickness_min", current_primitive->material.iridescence.thickness_min);
                shader_set_float(shader, "u_iridescence_factor", current_primitive->material.iridescence.factor);
                shader_set_float(shader, "u_iridescence_ior", current_primitive->material.iridescence.ior);
            }
            if(current_primitive->material.has_volume)
            {
                glActiveTexture(GL_TEXTURE7);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.volume.thickness_texture);
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

static inline void renderer_model_simpleDraw(const Model* model, Shader *shader, mat4 world_matrix)
{
    if (!model || !model->is_loaded || !model->meshes)
        return;

    // activate the shader
    shader_use(shader);
    // draw each primitive from a model
    for (uint32_t mi = 0; mi < model->mesh_count; mi++)
    {
        const Mesh* mesh = &model->meshes[mi];

        // multiply each model's mesh matrix by the world matrix of the entity that owns them
        mat4 final_transform;
        // avx makes EVERYTHING explode and i am too lazy to be bothered to fix it :D
        glm_mat4_mul_sse2(world_matrix, (vec4*)mesh->transform, final_transform);

        shader_set_mat4(shader, "u_model", final_transform);
        
        for(uint32_t pi = 0; pi < mesh->primitive_count; pi++)
        {
            const Primitive* current_primitive = &mesh->primitives[pi];

            if (current_primitive->material.pbr.albedo)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.pbr.albedo);
                shader_set_int(shader, "u_albedo", 0);
            }

            glActiveTexture(GL_TEXTURE0);

            glBindVertexArray(current_primitive->VAO);
            glDrawElements(GL_TRIANGLES, current_primitive->index_count, current_primitive->index_type, (void*)0);
            glBindVertexArray(0);
        }
    }
}

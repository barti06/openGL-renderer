#include "renderer.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <cimgui_impl.h>
#include "ui.h"

float quadVertices[] = { 
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
};

static inline void gbuffer_setup(Renderer* renderer, int w, int h)
{
    glGenFramebuffers(1, &renderer->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);

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

    GLuint attachments[5] = { 
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, 
        GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 
    };
    glDrawBuffers(5, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	    log_error("ERROR... renderer_init() says: FRAMEBUFFER INCOMPLETE.\n");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static inline void gbuffer_update(Renderer* renderer, int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);

    // main pass albedo texture
    glBindTexture(GL_TEXTURE_2D, renderer->g_albedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass position texture
    glBindTexture(GL_TEXTURE_2D, renderer->g_position);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass normal texture
    glBindTexture(GL_TEXTURE_2D, renderer->g_normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass orm (occlusion, roughness & metalness) texture
    glBindTexture(GL_TEXTURE_2D, renderer->g_orm);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // main pass depth texture
    glBindTexture(GL_TEXTURE_2D, renderer->g_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindTexture(GL_TEXTURE_2D, renderer->g_emissive);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderer_init(Renderer* renderer, Shader* shader, int viewportX,
    int viewportY, float nearZ, float farZ)
{
    renderer->nearZ = nearZ;
    renderer->farZ = farZ;
    renderer->viewportSize[0] = viewportX;
    renderer->viewportSize[1] = viewportY;
    renderer->active_shader = shader;

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

    shader_init(&renderer->quad_shader, "shaders/postfx.vert", "shaders/postfx.frag");

    gbuffer_setup(renderer, viewportX, viewportY);

    shader_use(&renderer->quad_shader);
    shader_set_int(&renderer->quad_shader, "g_position", 0);
    shader_set_int(&renderer->quad_shader, "g_normal", 1);
    shader_set_int(&renderer->quad_shader, "g_albedo", 2);
    shader_set_int(&renderer->quad_shader, "g_orm", 3);
    shader_set_int(&renderer->quad_shader, "g_emissive", 4);
    shader_set_int(&renderer->quad_shader, "g_depth", 5);

    glGenQueries(1, &renderer->geometry_query);
    glGenQueries(1, &renderer->light_query);

    // updates imgui timers four times a second
    renderer->stats_update_interval = 0.25f;
    renderer->stats_timer = 0.0f;
}

void renderer_updates(World* world, Renderer* renderer, int windowX, int windowY)
{
    if(renderer->viewportSize[0] != windowX || renderer->viewportSize[1] != windowY)
    {
        renderer->viewportSize[0] = windowX;
        renderer->viewportSize[1] = windowY;
        
        gbuffer_update(renderer, renderer->viewportSize[0], renderer->viewportSize[1]);
    }

    shader_use(renderer->active_shader);
    // send updated camera matrices to geometry shader
    shader_set_mat4(renderer->active_shader, "u_view", world->camera.view);
    shader_set_mat4(renderer->active_shader, "u_projection", world->camera.projection);

    shader_use(&renderer->quad_shader);
    shader_set_vec3(renderer->active_shader, "u_camera_position", world->camera.position);
    // update the camera
    camera_update_matrices(&world->camera, renderer->viewportSize, renderer->nearZ, renderer->farZ);
}

void renderer_draw_world(World* world, Renderer* renderer, double delta_time)
{
    glViewport(0, 0, renderer->viewportSize[0], renderer->viewportSize[1]);
    
    ui_begin();
    glBeginQuery(GL_TIME_ELAPSED, renderer->geometry_query);
    // gbuffer pass
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);
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

        if (renderable_has_flag(rc, RENDER_FLAG_NOCULL))
            glDisable(GL_CULL_FACE);
        if (renderable_has_flag(rc, RENDER_FLAG_WIREFRAME))
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        model_draw(rc->model, renderer->active_shader, world->transforms[index].world_matrix);

        if (renderable_has_flag(rc, RENDER_FLAG_NOCULL))
            glEnable(GL_CULL_FACE);
        if (renderable_has_flag(rc, RENDER_FLAG_WIREFRAME))
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEndQuery(GL_TIME_ELAPSED);

    glBeginQuery(GL_TIME_ELAPSED, renderer->light_query);
    // lighting pass
    glDisable(GL_DEPTH_TEST);
    shader_use(&renderer->quad_shader);
    // send pos texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->g_position);
    // send normal texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->g_normal);
    // send albedo texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderer->g_albedo);
    // send orm texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, renderer->g_orm);
    // send emissive texture
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, renderer->g_emissive);
    // send depth texture
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, renderer->g_depth);

    //draw quad
    glBindVertexArray(renderer->quad_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
    glEndQuery(GL_TIME_ELAPSED);

    GLuint64 time_geometry, time_light;
    glGetQueryObjectui64v(renderer->geometry_query, GL_QUERY_RESULT, &time_geometry);
    glGetQueryObjectui64v(renderer->light_query, GL_QUERY_RESULT, &time_light);

    float geometry_display = time_geometry / 1000000.0;
    float light_display = time_light / 1000000.0;
    renderer->stats_timer += delta_time;
    if (renderer->stats_timer >= renderer->stats_update_interval)
    {
        renderer->stats_fps = 1.0 / delta_time;
        renderer->stats_geometry_ms = geometry_display;
        renderer->stats_light_ms    = light_display;
        renderer->stats_timer       = 0.0f;
    }   
    igBegin("Renderer Statistics", NULL, 0);
    igText("FPS: %.3f", renderer->stats_fps);
    igText("Geometry Pass: %.3f ms", renderer->stats_geometry_ms);
    igText("Lighting Pass: %.3f ms", renderer->stats_light_ms);
    igEnd();
    ui_end();
}
#include "renderer.h"

void renderer_init(Renderer* renderer, Shader* shader, int viewportX,
    int viewportY, float nearZ, float farZ)
{
    renderer->nearZ = 0.1f;
    renderer->farZ = 1000.0f;
    renderer->viewportSize[0] = viewportX;
    renderer->viewportSize[1] = viewportY;
    renderer->active_shader = shader;
}

void renderer_updates(World* world, Renderer* renderer, int windowX, int windowY)
{
    renderer->viewportSize[0] = windowX;
    renderer->viewportSize[1] = windowY;
    shader_use(renderer->active_shader);

    // send updated camera matrices to main shader
    shader_set_mat4(renderer->active_shader, "u_view", world->camera.view);
    shader_set_mat4(renderer->active_shader, "u_projection", world->camera.projection);
    shader_set_vec3(renderer->active_shader, "u_camera_position", world->camera.position);

    // update the camera
    camera_update_matrices(&world->camera, renderer->viewportSize, renderer->nearZ, renderer->farZ);
}

void renderer_draw_world(World* world, Renderer* renderer)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, renderer->viewportSize[0], renderer->viewportSize[1]);

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
}
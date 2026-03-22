#include "world.h"
#include <basetsd.h>
#include <engine.h>

void engine_init(Engine* engine)
{
    init_glfw(&engine->window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		log_error("\nError at Engine::init. glad couldn't load!\n");
	}

    glViewport(0, 0, 1600, 900);
	glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glfwSwapInterval(1);
    engine->swap_interv = true;

    // init shader
	shader_init(&engine->shader, "shaders/vertex.vert", "shaders/fragment.frag");

    engine->camera = camera_init();
    engine->last_time = 0.0f; // init delta timing

    engine->nearZ = 0.1f;
    engine->farZ = 1000.0f;

    engine->canMove = false;
}

void engine_handleInput(Engine* engine)
{
    if(engine->canMove)
    {
        ActionType action = WALK;
        if(is_key_held(&engine->window, GLFW_KEY_LEFT_SHIFT))
            action = SPRINT;
        else if(is_key_held(&engine->window, GLFW_KEY_LEFT_CONTROL))
            action = CROUCH;
        if(is_key_held(&engine->window, GLFW_KEY_W))
            camera_process_movement(&engine->camera, engine->delta_time, FORWARD, action);

        if(is_key_held(&engine->window, GLFW_KEY_A))
            camera_process_movement(&engine->camera, engine->delta_time, LEFT, action);

        if(is_key_held(&engine->window, GLFW_KEY_S))
            camera_process_movement(&engine->camera, engine->delta_time, BACKWARD, action);

        if(is_key_held(&engine->window, GLFW_KEY_D))
            camera_process_movement(&engine->camera, engine->delta_time, RIGHT, action);

        if(is_key_held(&engine->window, GLFW_KEY_SPACE))
            camera_process_movement(&engine->camera, engine->delta_time, UP, action);

        if(is_key_held(&engine->window, GLFW_KEY_LEFT_CONTROL))
            camera_process_movement(&engine->camera, engine->delta_time, DOWN, action);

        camera_process_rotation(&engine->camera, engine->window.xoffset, engine->window.yoffset);
    }
    if(is_key_pressed(&engine->window, GLFW_KEY_P))
        glfwSetWindowShouldClose(engine->window.ptr, true);
    if(is_key_pressed(&engine->window, GLFW_KEY_ESCAPE))
    {
        engine->canMove = !engine->canMove;

        if(!engine->canMove)
            window_pause(&engine->window);
        else
            window_unpause(&engine->window);
    }
    if(is_key_pressed(&engine->window, GLFW_KEY_F5))
        shader_reload_frag(&engine->shader);
    if(is_key_pressed(&engine->window, GLFW_KEY_J))
    {
        engine->swap_interv = !engine->swap_interv;
        glfwSwapInterval(engine->swap_interv);
    }
} 

void engine_updates(Engine* engine)
{
    // delta time
    engine->current_time = glfwGetTime(); 
    engine->delta_time = engine->current_time - engine->last_time;
    engine->last_time = engine->current_time;

    engine_handleInput(engine);
    
    // set viewport size to full window size (for now at least)
    engine->viewportSize.x = engine->window.width;
    engine->viewportSize.y = engine->window.height;

    // update the camera
    camera_update_matrices(&engine->camera, 
        engine->viewportSize, 
        engine->nearZ, 
        engine->farZ);
}

void engine_draw(Engine* engine)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, engine->viewportSize.x, engine->viewportSize.y);
        
    shader_use(&engine->shader);

    // send updated camera matrices to main shader
    shader_set_mat4(&engine->shader, "u_view", engine->camera.view);
    shader_set_mat4(&engine->shader, "u_projection", engine->camera.projection);
    shader_set_vec3(&engine->shader, "u_camera_position", engine->camera.position);

    for (uint32_t index = 0; index < MAX_ENTITIES; index++) 
    {
        if (!engine->world->entities[index].active)
            continue;

        uint32_t needed = COMPONENT_TRANSFORM | COMPONENT_RENDERABLE;
        if (!entity_has_component(&engine->world->entities[index], needed))
            continue;

        RenderableComponent* rc = &engine->world->renderables[index];
        if (!renderable_has_flag(rc, RENDER_FLAG_VISIBLE))
            continue;

        if (renderable_has_flag(rc, RENDER_FLAG_NOCULL))
            glDisable(GL_CULL_FACE);
        if (renderable_has_flag(rc, RENDER_FLAG_WIREFRAME))
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        shader_set_mat4(&engine->shader, "u_model", engine->world->transforms[index].world_matrix);
        model_draw(rc->model, &engine->shader);

        if (renderable_has_flag(rc, RENDER_FLAG_NOCULL))
            glEnable(GL_CULL_FACE);
        if (renderable_has_flag(rc, RENDER_FLAG_WIREFRAME))
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void engine_loop(Engine* engine)
{
    engine->world = world_create();
    world_init(engine->world);
    while(!glfwWindowShouldClose(engine->window.ptr))
    {
        engine_updates(engine);
        
        log_info("FPS: %.2f", 1.0 / engine->delta_time);

        engine_draw(engine);

        window_update(&engine->window);

    }
}

void engine_cleanup(Engine* engine)
{
    world_destroy(engine->world);
    window_cleanup(&engine->window);
}
#include "entity.h"
#include "world.h"
#include <basetsd.h>
#include <engine.h>

void engine_init(Engine* engine)
{
    init_glfw(&engine->window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		log_error("\nERROR... engine_init() SAYS: COULDN'T LOAD GLAD!\n");
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
    
    world_update(engine->world, &engine->shader);

}

void engine_loop(Engine* engine)
{
    engine->world = world_create();
    
    //world_new_model(engine->world, "resources/sponza/Sponza.gltf", "sponza");
    world_new_model(engine->world, "resources/DamagedHelmet.glb", "helm");
    //world_new_model(engine->world, "resources/ABeautifulGame.glb", "chess");
    //world_new_model(engine->world, "resources/CarConcept.glb", "car");
    //world_new_model(engine->world, "resources/Bistro_Godot.glb", "bistro");
    //world_new_model(engine->world, "resources/porsche/scene.gltf", "porsche");
    //world_new_model(engine->world, "resources/DiffuseTransmissionTeacup.glb", "teacup");
    //world_new_model(engine->world, "resources/ToyCar.glb", "toycar");

    while(!glfwWindowShouldClose(engine->window.ptr))
    {
        engine_updates(engine);

        log_info("fps: %.2f", 1.0 / engine->delta_time);

        engine_draw(engine);

        window_update(&engine->window);
    }
}

void engine_cleanup(Engine* engine)
{
    world_destroy(engine->world);
    window_cleanup(&engine->window);
}
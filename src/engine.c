#include "engine.h"
#include <time.h>

#define DEFAULT_NEARZ 0.1f
#define DEFAULT_FARZ 1000.0f

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

    engine->world = world_create();
    world_new_model(engine->world, "resources/sponza/Sponza.gltf", "sponza");
    //world_new_model(engine->world, "resources/DamagedHelmet.glb", "helm");
    //world_new_model(engine->world, "resources/ABeautifulGame.glb", "chess");
    //world_new_model(engine->world, "resources/CarConcept.glb", "car");
    //world_new_model(engine->world, "resources/Bistro_Godot.glb", "bistro");
    //world_new_model(engine->world, "resources/porsche/scene.gltf", "porsche");
    //world_new_model(engine->world, "resources/DiffuseTransmissionTeacup.glb", "teacup");
    //world_new_model(engine->world, "resources/ToyCar.glb", "toycar");
    engine->world->camera = camera_init();
    renderer_init(&engine->renderer, &engine->shader, engine->window.width, engine->window.width, DEFAULT_NEARZ, DEFAULT_FARZ);

    engine->last_time = 0.0f; // init delta timing
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
            camera_process_movement(&engine->world->camera, engine->delta_time, FORWARD, action);

        if(is_key_held(&engine->window, GLFW_KEY_A))
            camera_process_movement(&engine->world->camera, engine->delta_time, LEFT, action);

        if(is_key_held(&engine->window, GLFW_KEY_S))
            camera_process_movement(&engine->world->camera, engine->delta_time, BACKWARD, action);

        if(is_key_held(&engine->window, GLFW_KEY_D))
            camera_process_movement(&engine->world->camera, engine->delta_time, RIGHT, action);

        if(is_key_held(&engine->window, GLFW_KEY_SPACE))
            camera_process_movement(&engine->world->camera, engine->delta_time, UP, action);

        if(is_key_held(&engine->window, GLFW_KEY_LEFT_CONTROL))
            camera_process_movement(&engine->world->camera, engine->delta_time, DOWN, action);

        camera_process_rotation(&engine->world->camera, engine->window.xoffset, engine->window.yoffset);
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

    world_update(engine->world);

    renderer_updates(engine->world, &engine->renderer, engine->window.width, engine->window.height);
}

void engine_draw(Engine* engine)
{
    renderer_draw_world(engine->world, &engine->renderer);
}
void engine_loop(Engine* engine)
{    

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
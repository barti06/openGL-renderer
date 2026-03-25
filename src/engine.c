#include "engine.h"
#include "ui.h"

#define DEFAULT_NEARZ 0.1f
#define DEFAULT_FARZ 100.0f

static inline void engine_send_lights(Engine* engine);

void engine_init(Engine* engine)
{
    init_glfw(&engine->window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		log_error("\nERROR... engine_init() SAYS: COULDN'T LOAD GLAD!\n");
	}

    glViewport(0, 0, 1600, 900);
	glEnable(GL_DEPTH_TEST);

    glfwSwapInterval(1);
    engine->swap_interv = true;

    // init shader
	shader_init(&engine->shader, "shaders/vertex.vert", "shaders/fragment.frag");

    engine->world = world_create();
    world_new_model(engine->world, "resources/sponza/Sponza.gltf", "sponza");
    //world_new_model(engine->world, "resources/sponzapbr.glb", "sponzapbr");
    //world_new_model(engine->world, "resources/DamagedHelmet.glb", "helm");
    //world_new_model(engine->world, "resources/ABeautifulGame.glb", "chess");
    //world_new_model(engine->world, "resources/CarConcept.glb", "car");
    //world_new_model(engine->world, "resources/Bistro_Godot.glb", "bistro");
    //world_new_model(engine->world, "resources/porsche/scene.gltf", "porsche");
    //world_new_model(engine->world, "resources/DiffuseTransmissionTeacup.glb", "teacup");
    //world_new_model(engine->world, "resources/ToyCar.glb", "toycar");
    //world_new_model(engine->world, "resources/MultiUVTest.glb", "multiUVtest");

    world_new_light(engine->world, LIGHT_TYPE_POINT, "myLight");
    engine->pointlight_count = 1;

    engine->world->camera = camera_init();
    renderer_init(&engine->renderer, &engine->shader, engine->window.width, engine->window.height, DEFAULT_NEARZ, DEFAULT_FARZ);

    engine->last_time = 0.0f; // init delta timing
    engine->canMove = false;
    ui_init(engine->window.ptr);
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
    if(is_key_pressed(&engine->window, GLFW_KEY_F6))
        renderer_gbuffer_reload(&engine->renderer);
    if(is_key_pressed(&engine->window, GLFW_KEY_F7))
        renderer_postfx_reload(&engine->renderer);

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

    engine_send_lights(engine);

    engine_handleInput(engine);

    world_update(engine->world);

    renderer_updates(engine->world, &engine->renderer, engine->window.width, engine->window.height);
}

void engine_draw(Engine* engine)
{
    ui_begin();
    renderer_draw_world(engine->world, &engine->renderer, engine->delta_time);
    renderer_ui(&engine->renderer);
    world_ui(engine->world);
    ui_end();
}

void engine_loop(Engine* engine)
{    

    while(!glfwWindowShouldClose(engine->window.ptr))
    {
        engine_updates(engine);

        engine_draw(engine);

        window_update(&engine->window);
    }
}

void engine_cleanup(Engine* engine)
{
    world_destroy(engine->world);
    window_cleanup(&engine->window);
}

void shader_update_light(Shader* shader, World* wl, int32_t index, int32_t* point_ind, int32_t* spot_ind)
{
    char uniform_name[32];
    switch(wl->lights[index].light_type)
    {
        case LIGHT_TYPE_POINT:
        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].position", *point_ind);
        shader_set_vec3(shader, uniform_name, wl->transforms[index].position);

        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].quadratic", *point_ind);
        shader_set_float(shader, uniform_name, wl->lights[index].lights.point.quadratic);
        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].linear", index);
        shader_set_float(shader, uniform_name, wl->lights[index].lights.point.linear);
        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].intensity", index);
        shader_set_float(shader, uniform_name, wl->lights[index].intensity);

        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].diffuse", *point_ind);
        shader_set_vec3(shader, uniform_name, wl->lights[index].lights.point.diffuse);
        (*point_ind)++;
        break;

// TODO: finish spot light support 
        case LIGHT_TYPE_SPOT:
        snprintf(uniform_name, sizeof(uniform_name), "u_spotlights[%u].position", *spot_ind);
        shader_set_vec3(shader, uniform_name, wl->transforms[index].position);

        snprintf(uniform_name, sizeof(uniform_name), "u_spotlights[%u].quadratic", *spot_ind);
        shader_set_float(shader, uniform_name, wl->lights[index].lights.spot.quadratic);
        snprintf(uniform_name, sizeof(uniform_name), "u_spotlights[%u].linear", index);
        shader_set_float(shader, uniform_name, wl->lights[index].lights.spot.linear);

        snprintf(uniform_name, sizeof(uniform_name), "u_spotlights[%u].diffuse", *spot_ind);
        shader_set_vec3(shader, uniform_name, wl->lights[index].lights.spot.diffuse);
        (*spot_ind)++;
        break;

        default:
        break;
    }
}

void engine_send_lights(Engine* engine)
{
    int32_t point_index = 0;
    int32_t spot_index = 0;
    shader_use(&engine->renderer.light_shader);
    for(size_t index = 0; index < engine->world->entities_count; index++)
    {
        if(!engine->world->entities[index].active)
            continue;
        if (!entity_has_component(&engine->world->entities[index], COMPONENT_LIGHT))
            continue;

        shader_update_light(&engine->renderer.light_shader, engine->world, index, &point_index, &spot_index);
    }
    shader_set_int(&engine->renderer.light_shader, "u_pointLight_count", point_index);
    shader_set_int(&engine->renderer.light_shader, "u_spotLight_count", spot_index);
}
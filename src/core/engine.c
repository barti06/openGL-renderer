#include "engine.h"
#include "ui.h"
#include <string.h>

// check for window key inputs
static inline void engine_handleInput(Engine* engine);

// handles engine updates 
// (mouse movement, keyboard input, delta time, etc)
static inline void engine_updates(Engine* engine);

// engine draw function (may be moved later on)
static inline void engine_draw(Engine* engine);

static inline void engine_send_lights(Engine* engine);

void engine_init(Engine *engine, int argc, char *argv[])
{
    int32_t w = 0;
    int32_t h = 0;

    for(int i = 1; i < argc; i++)
    {
        // WHY does strcmp returns zero for matches
        if(!strcmp(argv[i], "-w") || !strcmp(argv[i], "--width"))
            w = atoi(argv[i + 1]);
        if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--height"))
            h = atoi(argv[i + 1]);
    }

    Window *win = &engine->window;
    window_init(win, w, h);

    renderer_init(&engine->renderer, win->width, win->height);

    engine->last_time = 0.0f; // init delta timing
    engine->canMove = false;
    ui_init(engine->window.ptr);

    engine->swap_interv = true;
    glfwSwapInterval(engine->swap_interv);

    engine->world = world_create();
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

static inline void engine_handleInput(Engine* engine)
{
    if(engine->world->current_event == WORLD_EVENT_DISABLE_INPUT)
        return;
    Window *w = &engine->window;
    Camera *c = &engine->world->camera;
    // all of this should be its own function
    if(engine->canMove)
    {
        ActionType action = WALK;
        if(is_key_held(w, GLFW_KEY_LEFT_SHIFT))
            action = SPRINT;
        else if(is_key_held(w, GLFW_KEY_LEFT_CONTROL))
            action = CROUCH;

        if(is_key_held(w, GLFW_KEY_W))
            camera_process_movement(c, engine->delta_time, FORWARD, action);
        if(is_key_held(w, GLFW_KEY_A))
            camera_process_movement(c, engine->delta_time, LEFT, action);
        if(is_key_held(w, GLFW_KEY_S))
            camera_process_movement(c, engine->delta_time, BACKWARD, action);
        if(is_key_held(w, GLFW_KEY_D))
            camera_process_movement(c, engine->delta_time, RIGHT, action);
        if(is_key_held(w, GLFW_KEY_SPACE))
            camera_process_movement(c, engine->delta_time, UP, action);
        if(is_key_held(w, GLFW_KEY_LEFT_CONTROL))
            camera_process_movement(c, engine->delta_time, DOWN, action);

        camera_process_rotation(c, w->xoffset, w->yoffset);
    }

    if(is_key_pressed(w, GLFW_KEY_P))
        glfwSetWindowShouldClose(engine->window.ptr, true);

    if(is_key_pressed(w, GLFW_KEY_ESCAPE))
    {
        engine->canMove = !engine->canMove;

        if(!engine->canMove)
            window_pause(w);
        else
            window_unpause(w);
    }

    if(is_key_pressed(w, GLFW_KEY_F5))
        shader_reload_frag(engine->renderer.active_shader);
    if(is_key_pressed(&engine->window, GLFW_KEY_F6))
        renderer_gbuffer_reload(&engine->renderer);
    if(is_key_pressed(w, GLFW_KEY_F7))
        renderer_postfx_reload(&engine->renderer);

    if(is_key_pressed(w, GLFW_KEY_J))
    {
        engine->swap_interv = !engine->swap_interv;
        glfwSwapInterval(engine->swap_interv);
    }
} 

static inline void engine_updates(Engine* engine)
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

static inline void engine_draw(Engine* engine)
{
    ui_begin();
    renderer_draw_world(engine->world, &engine->renderer, engine->delta_time);
    renderer_ui(&engine->renderer);
    world_ui(engine->world);
    ui_end();
}

static inline void shader_update_light(Shader* shader, World* wl, int32_t index, int32_t* point_ind, int32_t* spot_ind)
{
    char uniform_name[32];

    switch(wl->lights[index].light_type)
    {
        case LIGHT_TYPE_POINT:
        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].position", *point_ind);
        shader_set_vec3(shader, uniform_name, wl->transforms[index].position);

        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].quadratic", *point_ind);
        shader_set_float(shader, uniform_name, wl->lights[index].lights.point.quadratic);
        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].linear", *point_ind);
        shader_set_float(shader, uniform_name, wl->lights[index].lights.point.linear);
        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].intensity", *point_ind);
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
        snprintf(uniform_name, sizeof(uniform_name), "u_spotlights[%u].linear", *spot_ind);
        shader_set_float(shader, uniform_name, wl->lights[index].lights.spot.linear);

        snprintf(uniform_name, sizeof(uniform_name), "u_spotlights[%u].diffuse", *spot_ind);
        shader_set_vec3(shader, uniform_name, wl->lights[index].lights.spot.diffuse);
        (*spot_ind)++;
        break;
        default:
        break;
    }
}

static inline void engine_send_lights(Engine* engine)
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

    if(engine->world->enable_directional_light)
    {
        // dir light uses the rotation from the transform component to get its direction
        shader_set_vec3(&engine->renderer.light_shader, "u_dirlight.direction", engine->world->dirlight_dir);
        shader_set_vec3(&engine->renderer.light_shader, "u_dirlight.diffuse", engine->world->directional.diffuse);
        shader_set_float(&engine->renderer.light_shader, "u_dirlight.intensity", engine->world->dirlight_inten);
    }

    shader_set_bool(&engine->renderer.light_shader, "u_dirlight_enabled", engine->world->enable_directional_light);
    shader_set_int(&engine->renderer.light_shader, "u_pointLight_count", point_index);
    shader_set_int(&engine->renderer.light_shader, "u_spotLight_count", spot_index);
}

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

static inline void renderer_ui(Renderer* renderer)
{
    igBegin("Renderer", NULL, 0);
    igText("FPS: %.3f", renderer->stats_fps);
    igText("Geometry pass: %.3f ms", renderer->stats_geometry_ms);
    igText("Lighting pass: %.3f ms", renderer->stats_light_ms);
    igText("Post-Processing pass: %.3f ms", renderer->stats_fx_ms);
    igSeparator();
    igCombo_Str_arr("Current gbuffer view", &renderer->gbuffer_view, gbuffer_options, 9, -1);
    igSeparator();
    igCombo_Str_arr("Current tone mapper", &renderer->tonemap, tonemap_options, 3, -1);
    igSliderFloat("Gamma", &renderer->gamma, 0.0f, 3.0f, "%.1f", 0);
    igSliderFloat("Exposure", &renderer->exposure, 0.0f, 2.0f, "%.1f", 0);
    igSliderFloat("Brightness", &renderer->brightness, 0.0f, 2.0f, "%.1f", 0);
    igCheckbox("Enable vignette", &renderer->vignette_enabled);
    if(renderer->vignette_enabled)
        igSliderFloat("Vignette strength", &renderer->vignette_strength, 0.0f, 2.0f, "%.1f", 0);
    igCheckbox("Enable chromatic aberration", &renderer->CA_enabled);
    if(renderer->CA_enabled)
        igSliderFloat("Chromatic aberration strength", &renderer->CA_strength, 0.0f, 4.0f, "%.1f", 0);
    igEnd();
}

static inline void world_ui(World* world)
{
    igBegin("World", NULL, 0);
    igText("Entities: %u / %u", world->entities_count, MAX_ENTITIES);
    igText("Models: %u / %u", world->model_count, MAX_MODELS);
    igSeparator();

    for (uint32_t i = 0; i < world->entities_count; i++)
    {
        Entity* entity = &world->entities[i];
        if (!entity->active)
            continue;

        // one collapsing header per entity — use id as unique push to
        // avoid name collisions between entities with the same name
        igPushID_Int((int)i);
        bool open = igCollapsingHeader_TreeNodeFlags(entity->name, 0);
        if(open)
        {
            // transformation ui
            if(entity_has_component(entity, COMPONENT_TRANSFORM))
            {
                TransformComponent* t = &world->transforms[i];
                igText("Transform");
                igIndent(8.0f);

                if (igDragFloat3("Position", t->position, 0.01f, -1000.0f, 1000.0f, "%.3f", 0))
                    t->dirty = true;

                if (igDragFloat3("Rotation", t->rotation, 0.5f, -360.0f, 360.0f, "%.1f", 0))
                    t->dirty = true;

                if (igDragFloat3("Scale", t->scale, 0.01f, 0.001f, 1000.0f, "%.3f", 0))
                    t->dirty = true;

                // uniform scale convenience slider
                igPushID_Str("uniform_scale");
                float uniform = t->scale[0];
                if (igDragFloat("Uniform scale", &uniform, 0.01f, 0.001f, 1000.0f, "%.3f", 0))
                    transform_set_scale(t, uniform);
                igPopID();

                igUnindent(8.0f);
                igSeparator();
            }

            // renderable ui
            if (entity_has_component(entity, COMPONENT_RENDERABLE))
            {
                RenderableComponent* rc = &world->renderables[i];
                igText("Renderable");
                igIndent(8.0f);

                bool visible = renderable_has_flag(rc, RENDER_FLAG_VISIBLE);
                bool wireframe = renderable_has_flag(rc, RENDER_FLAG_WIREFRAME);
                bool nocull = renderable_has_flag(rc, RENDER_FLAG_NOCULL);

                if (igCheckbox("Visible", &visible))
                    renderable_toggle_flag(rc, RENDER_FLAG_VISIBLE);
                if (igCheckbox("Wireframe", &wireframe))
                    renderable_toggle_flag(rc, RENDER_FLAG_WIREFRAME);
                if (igCheckbox("No cull",   &nocull))
                    renderable_toggle_flag(rc, RENDER_FLAG_NOCULL);

                if (rc->model)
                    igText("Meshes: %u  Primitives: %u", rc->model->mesh_count, rc->model->model_primitive_count);

                igUnindent(8.0f);
                igSeparator();
            }

            // light ui
            if (entity_has_component(entity, COMPONENT_LIGHT))
            {
                LightComponent* lc = &world->lights[i];
                igText("Light");
                igIndent(8.0f);

                const char* light_type_names[] = { 
                    "Point", 
                    "Spot", 
                    "Directional" 
                };
                igText("Type: %s", light_type_names[lc->light_type]);
                igDragFloat("Intensity", &lc->intensity, 0.01f, 0.0f, 1000.0f, "%.2f", 0);

                switch (lc->light_type)
                {
                    case LIGHT_TYPE_POINT:
                    {
                        PointLight* p = &lc->lights.point;
                        igColorEdit3("Color", p->diffuse, 0);
                        igDragFloat("Linear", &p->linear,    0.001f, 0.0f, 1.0f, "%.4f", 0);
                        igDragFloat("Quadratic", &p->quadratic, 0.001f, 0.0f, 1.0f, "%.4f", 0);
                        break;
                    }
                    case LIGHT_TYPE_SPOT:
                    {
                        SpotLight* sl = &lc->lights.spot;
                        igColorEdit3("Color", sl->diffuse, 0);
                        igDragFloat("Linear", &sl->linear,    0.001f, 0.0f,   1.0f, "%.4f", 0);
                        igDragFloat("Quadratic", &sl->quadratic, 0.001f, 0.0f,   1.0f, "%.4f", 0);

                        // display in degrees — convert back to cos on change
                        float inner_deg = glm_deg(acosf(sl->inner_cutoff));
                        float outer_deg = glm_deg(acosf(sl->outer_cutoff));
                        if (igDragFloat("Inner cutoff°", &inner_deg, 0.5f, 0.0f, 90.0f, "%.1f", 0))
                            sl->inner_cutoff = cosf(glm_rad(inner_deg));
                        if (igDragFloat("Outer cutoff°", &outer_deg, 0.5f, 0.0f, 90.0f, "%.1f", 0))
                            sl->outer_cutoff = cosf(glm_rad(outer_deg));
                        break;
                    }
                    case LIGHT_TYPE_DIRECTIONAL:
                    {
                        DirectionalLight* dl = &lc->lights.directional;
                        igColorEdit3("Color", dl->diffuse, 0);
                        break;
                    }
                    default: break;
                }
                igUnindent(8.0f);
            }
        }

        igPopID();
    }
    igEnd();
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
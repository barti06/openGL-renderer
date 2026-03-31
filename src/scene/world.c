#include "world.h"
#include "ui.h"
#include "utils.h"
#include <log.h>
#include <string.h>

static inline Model* world_add_model(World* world, const char* path);
static inline void world_add_light(World* world, EntityID id, LightType type);
static inline void world_add_transform(World* world, EntityID id);
static inline void world_add_renderable(World* world, EntityID id, Model* model);


static inline void world_new_model(World* world, const char* path, const char* name);
static inline void world_new_light(World* world, LightType type, const char* name);

World* world_create(void)
{
#ifdef _WIN32
    World* world = _aligned_malloc(sizeof(World), 32);
#else
    World* world = aligned_alloc(32, sizeof(World));
#endif

    if(!world)
    {
        log_error("ERROR... world_create() SAYS: OUT OF MEMORY.");
        return NULL;
    }

    memset(world, 0, sizeof(World));

    world->camera = camera_init();
    world->current_event = WORLD_EVENT_NONE;
    world->dirlight_inten = 1.0f;
    glm_vec3_dup((vec3)GLM_VEC3_ONE_INIT, world->directional.diffuse);
    glm_vec3_dup(((vec3){0.25f, -1.0f, -0.15f}), world->dirlight_dir);
    world->enable_directional_light = true;
    world->update_shadow = true;

    world->entities_count = 0;
    return world;
}

EntityID world_create_entity(World* world, const char* name)
{
    for(uint64_t index = 0; index < MAX_ENTITIES; index++)
    {
        if(!world->entities[index].active)
        {
            entity_init(&world->entities[index], index, name);
            world->entities[index].active = true;
            world->entities_count++;
            return index;
        }
    }
    return INVALID_ENTITY_ID;
}

void world_update(World* world)
{
    for(size_t index = 0; index < world->entities_count; index++)
    {
        if(!world->entities[index].active)
            continue;
        if (!entity_has_component(&world->entities[index], COMPONENT_TRANSFORM))
            continue;
        transform_update(&world->transforms[index]);
    }
}

void world_destroy(World *world)
{
    if(!world)
        return;

    for(uint32_t index = 0; index < world->model_count; index++)
    {
        model_free(&world->models[index]);
    }

#ifdef _WIN32
    _aligned_free(world);
#else
    free(world);
#endif
}

const char* light_type_names[] = { 
    "Point", 
    "Spot"
};

static struct
{
    bool open;
    char path[MAX_MODEL_PATH_LENGTH];
    char name[ENTITY_NAME_MAX_LENGTH];
    bool path_chosen;
} add_model_s = {0};

static struct
{
    bool open;
    LightType lt;
    char name[ENTITY_NAME_MAX_LENGTH];
} add_light_s = {0};

void world_ui(World* world)
{
    igBegin("World", NULL, 0);
    igText("Position:\n X=%.2f, Y=%.2f, Z=%.2f", world->camera.position[0], world->camera.position[2], world->camera.position[2]);
    igText("Facing:\n X=%.2f, Y=%.2f, Z=%.2f", world->camera.front[0], world->camera.front[1], world->camera.front[2]);
    igText("Entities: %u / %u", world->entities_count, MAX_ENTITIES);
    igText("Models: %u / %u", world->model_count, MAX_MODELS);
    igSeparator();

    igCheckbox("Enable directional lighting", &world->enable_directional_light);
    if(world->enable_directional_light)
    {
        igColorEdit3("Dirlight color", world->directional.diffuse, 0);
        if(igSliderFloat3("Dirlight direction", world->dirlight_dir, -1.0f, 1.0f, "%.2f", 0))
            world->update_shadow = true;
        igDragFloat("Dirlight intensity", &world->dirlight_inten, 0.1f, 0.0f, 250.0f, "%.1f", 0);
    }
    igSeparator();
    // get screen center to draw the new popup window there
    ImVec2 center;
    ImGuiIO* io = igGetIO_Nil();
    center.x = io->DisplaySize.x * 0.5f;
    center.y = io->DisplaySize.y * 0.5f;

    // add model button
    if(igButton("Add model", (ImVec2){0,0}))
    {
        add_model_s.open = true;
        add_model_s.path[0] = '\0';
        add_model_s.name[0] = '\0';
        add_model_s.path_chosen = true;
    }
    if(add_model_s.open)
        igOpenPopup_Str("Add model##popup", 0);

    igSetNextWindowPos(center, ImGuiCond_Appearing, (ImVec2){0.5f, 0.5f});
    igSetNextWindowSize((ImVec2){420, 0}, ImGuiCond_Appearing);

    if(igBeginPopupModal("Add model##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        world->current_event = WORLD_EVENT_DISABLE_INPUT;
        // filepath loader
        igText("Filepath: %s", add_model_s.path_chosen ? add_model_s.path : "(none)");
        if(igButton("Browse...", (ImVec2){0,0}))
        {
            char fpath[MAX_MODEL_PATH_LENGTH];
            if(open_file_dialog(fpath, sizeof(fpath)))
            {
                strncpy(add_model_s.path, fpath, sizeof(add_model_s.path) - 1);
                add_model_s.path_chosen = true;
            }
            else
            {
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.0f, 0.3f, 0.3f, 1.0f});
                igText("ERROR... FAILED TO LOAD FROM FILE DIALOG!");
                igPopStyleColor(1);
            }
        }
        
        // entity name loader
        igSpacing();
        igText("Entity name:");
        igInputText("##entity_name", add_model_s.name, sizeof(add_model_s.name), 0, NULL, NULL);

        bool can_load = add_model_s.path_chosen && add_model_s.name[0] != '\0';

        if (!can_load)
            igBeginDisabled(true);

        if(igButton("Load",(ImVec2){120, 0}))
        {
            if(world->model_count >= MAX_MODELS)
            {
                igText("ERROR... CAN'T LOAD MORE MODELS!");
            }
            else if(world->entities_count >= MAX_ENTITIES)
            {
                igText("ERROR... CAN'T LOAD MORE ENTITIES!");
            }
            else
            {
                world_new_model(world, add_model_s.path, add_model_s.name);
                world->update_shadow = true;
            }
            add_model_s.open = false;
            world->current_event = WORLD_EVENT_NONE;
            igCloseCurrentPopup();
        }

        if(!can_load)
            igEndDisabled();

        if(igButton("Close", (ImVec2){120, 0}))
        {
            add_model_s.open = false;
            world->current_event = WORLD_EVENT_NONE;
            igCloseCurrentPopup();
        }

        igEndPopup();
    }

    // only one dirlight can exist so i got it out of here
    // add light button
    if(igButton("Add light", (ImVec2){0,0}))
    {
        add_light_s.open = true;
        add_light_s.name[0] = '\0';
        add_light_s.lt = LIGHT_TYPE_POINT;
    }
    if(add_light_s.open)
        igOpenPopup_Str("Add light##popup", 0);

    igSetNextWindowPos(center, ImGuiCond_Appearing, (ImVec2){0.5f, 0.5f});
    igSetNextWindowSize((ImVec2){420, 0}, ImGuiCond_Appearing);

    // add light popup window
    if(igBeginPopupModal("Add light##popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        world->current_event = WORLD_EVENT_DISABLE_INPUT;
        
        // entity light type loader
        igCombo_Str_arr("Light type", &add_light_s.lt, light_type_names, 2, -1);

        // entity name loader
        igSpacing();
        igText("Entity name:");
        igInputText("##entity_name", add_light_s.name, sizeof(add_light_s.name), 0, NULL, NULL);

        bool can_load = add_light_s.name[0] != '\0';

        if (!can_load)
            igBeginDisabled(true);

        if(igButton("Load",(ImVec2){120, 0}))
        {
            if(world->entities_count >= MAX_ENTITIES)
                igText("ERROR... CAN'T LOAD MORE ENTITIES!");
            else
            {
                world_new_light(world, add_light_s.lt, add_light_s.name);
                add_light_s.open = false;
                world->current_event = WORLD_EVENT_NONE;
                igCloseCurrentPopup();
            }
        }

        if(!can_load)
            igEndDisabled();

        if(igButton("Close", (ImVec2){120, 0}))
        {
            add_light_s.open = false;
            world->current_event = WORLD_EVENT_NONE;
            igCloseCurrentPopup();
        }

        igEndPopup();
    }

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
                bool culling = renderable_has_flag(rc, RENDER_FLAG_CULL);
                bool blending = renderable_has_flag(rc, RENDER_FLAG_BLEND);

                if (igCheckbox("Visible", &visible))
                    renderable_toggle_flag(rc, RENDER_FLAG_VISIBLE);
                if (igCheckbox("Wireframe", &wireframe))
                    renderable_toggle_flag(rc, RENDER_FLAG_WIREFRAME);
                if (igCheckbox("Culling",   &culling))
                    renderable_toggle_flag(rc, RENDER_FLAG_CULL);
                if (igCheckbox("Blending", &blending))
                    renderable_toggle_flag(rc, RENDER_FLAG_BLEND);

                if (rc->model)
                {
                    igText("Meshes: %u  Primitives: %u", rc->model->mesh_count, rc->model->model_primitive_count);
                    igText("Model path: %s", rc->model->filepath);
                }  
                igUnindent(8.0f);
                igSeparator();
            }

            // light ui
            if (entity_has_component(entity, COMPONENT_LIGHT))
            {
                LightComponent* lc = &world->lights[i];
                igText("Light");
                igIndent(8.0f);

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
                    default: 
                    break;
                }
                igUnindent(8.0f);
            }
        }

        igPopID();
    }
    igEnd();
}

static inline Model* world_add_model(World* world, const char* path)
{
    if(world->model_count >= MAX_MODELS)
    {
        log_error("ERROR... world_add_model() SAYS: TOO MANY MODELS IN A SINGLE WORLD!");
        return NULL;
    }
    uint32_t id = world->model_count++;
    model_load(&world->models[id], path);
    return &world->models[id];
}

static inline void world_add_transform(World* world, EntityID id)
{
    if(id > MAX_ENTITIES - 1)
        return;

    entity_add_component(&world->entities[id], COMPONENT_TRANSFORM);

    transform_init(&world->transforms[id]);

    return;
}

static inline void world_add_renderable(World* world, EntityID id, Model* model)
{
    if(id > MAX_ENTITIES - 1)
        return;
    
    entity_add_component(&world->entities[id], COMPONENT_RENDERABLE);

    renderable_init(&world->renderables[id], model);

    return;
}

static inline void world_add_light(World* world, EntityID id, LightType type)
{
    if(id > MAX_ENTITIES - 1)
        return;
    
    entity_add_component(&world->entities[id], COMPONENT_LIGHT);

    switch(type)
    {
        case LIGHT_TYPE_POINT:
        world->lights[id] = light_init_point();
        break;
        case LIGHT_TYPE_SPOT:
        world->lights[id] = light_init_spot();
        break;
        // default to a point light
        default:
        world->lights[id] = light_init_point();
        break;
    }
    return;
}


static inline void world_new_model(World* world, const char* path, const char* name)
{
    log_info("world_new_model() SAYS: ADDING NEW MODEL ENTITY...");

    Model* m = world_add_model(world, path);

    EntityID e = world_create_entity(world, name);

    world_add_transform(world, e);

    world_add_renderable(world, e, m);

    log_info("world_new_model() SAYS: ADDED NEW MODEL ENTITY!");
}

static inline void world_new_light(World* world, LightType type, const char* name)
{
    log_info("world_new_light() SAYS: ADDING NEW LIGHT ENTITY...");

    EntityID e = world_create_entity(world, name);

    world_add_transform(world, e);

    world_add_light(world, e, type);

    log_info("world_new_light() SAYS: ADDED NEW LIGHT ENTITY!");
}

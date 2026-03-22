#include "world.h"
#include "entity.h"
#include "model.h"
#include <basetsd.h>
#include <stdint.h>

World* world_create(void)
{
    World* world = calloc(1, sizeof(World));
    if(!world)
    {
        log_error("ERROR... world_create() SAYS: OUT OF MEMORY.");
        return NULL;
    }

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

static inline Model* world_add_model(World* world, const char* path)
{
    uint32_t id = world->model_count++;
    model_load(&world->models[id], path);
    return &world->models[id];
}

static inline TransformComponent* world_add_transform(World* world, EntityID id)
{
    entity_add_component(&world->entities[id], COMPONENT_TRANSFORM);
    transform_init(&world->transforms[id]);
    return &world->transforms[id];
}

static inline RenderableComponent* world_add_renderable(World* world, EntityID id, Model* model)
{
    entity_add_component(&world->entities[id], COMPONENT_RENDERABLE);
    renderable_init(&world->renderables[id], model);
    return &world->renderables[id];
}

void world_init(World* world)
{
    Model* model = world_add_model(world, "resources/DamagedHelmet.glb");

    EntityID e = world_create_entity(world, "helmet");
    TransformComponent* tc = world_add_transform(world, e);
    RenderableComponent* rc = world_add_renderable(world, e, model);
    transform_set_scale_3float(tc, 10.0f, 10.0f, 10.0f);
}

static inline void world_update(World* world)
{
    for(uint64_t index = 0; index < MAX_ENTITIES; index++)
    {
        if (!world->entities[index].active)
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
    free(world);
}
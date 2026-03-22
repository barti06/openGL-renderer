#include "world.h"
#include "entity.h"
#include "model.h"
#include <basetsd.h>
#include <stdint.h>
#include <string.h>

World* world_create(void)
{
#ifdef _WIN32
    World* world = _aligned_malloc(sizeof(World), 32);
#else
    World* world = aligned_malloc(32, sizeof(World));
#endif

    if(!world)
    {
        log_error("ERROR... world_create() SAYS: OUT OF MEMORY.");
        return NULL;
    }

    memset(world, 0, sizeof(World));

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

static inline TransformComponent* world_add_transform(World* world, EntityID id)
{
    if(id > MAX_ENTITIES)
        return NULL;

    entity_add_component(&world->entities[id], COMPONENT_TRANSFORM);

    transform_init(&world->transforms[id]);

    return &world->transforms[id];
}

static inline RenderableComponent* world_add_renderable(World* world, EntityID id, Model* model)
{
    if(id > MAX_ENTITIES)
        return NULL;
    
    entity_add_component(&world->entities[id], COMPONENT_RENDERABLE);

    renderable_init(&world->renderables[id], model);

    return &world->renderables[id];
}

void world_update(World* world)
{
    for(uint64_t index = 0; index < world->entities_count; index++)
    {
        if(!world->entities[index].active)
            continue;
        if (!entity_has_component(&world->entities[index], COMPONENT_TRANSFORM))
            continue;
        transform_update(&world->transforms[index]);
    }
}

void world_new_model(World* world, const char* path, const char* name, vec3 scale)
{
    log_info("world_new_model() SAYS: ADDING NEW MODEL ENTITY...");

    Model* m = world_add_model(world, path);

    EntityID e = world_create_entity(world, name);

    TransformComponent* tc = world_add_transform(world, e);
    transform_set_scale_vec3(tc, scale);

    RenderableComponent* rc = world_add_renderable(world, e, m);

    log_info("world_new_model() SAYS: ADDED NEW MODEL ENTITY!");
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
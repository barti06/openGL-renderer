#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include "camera.h"
#include <stddef.h>
#include <assert.h>

#define MAX_ENTITIES 1024
#define MAX_MODELS 64

typedef struct World
{
    Entity entities[MAX_ENTITIES];
    uint32_t entities_count;

    // storage
    TransformComponent transforms[MAX_ENTITIES];
    RenderableComponent renderables[MAX_ENTITIES];
    LightComponent lights[MAX_ENTITIES];
    
    Model models[MAX_MODELS];
    uint32_t model_count;

    Camera camera;
} World;

// heap allocates a world (they are very heavy to always have on stack)
World* world_create(void);
void world_update(World* world);
void world_new_model(World* world, const char* path, const char* name);
void world_new_light(World* world, LightType type, const char* name);
void world_destroy(World* world);
#endif
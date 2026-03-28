#ifndef ENTITY_H
#define ENTITY_H

#include "model.h"

typedef uint64_t EntityID;
#define INVALID_ENTITY_ID ((EntityID)-1)

typedef enum ComponentType
{
    COMPONENT_TRANSFORM = 1 << 0,
    COMPONENT_RENDERABLE = 1 << 2,
    COMPONENT_LIGHT = 1 << 3
} ComponentType;

// this one is quite the FAT struct -__-
typedef struct TransformComponent
{
    mat4 world_matrix;
    vec3 position;
    vec3 rotation;
    vec3 scale;
    float scale_1d;
    bool dirty; // check if model needs update
} TransformComponent;
 
typedef enum RenderFlags
{
    RENDER_FLAG_VISIBLE = 1 << 0,
    RENDER_FLAG_WIREFRAME = 1 << 1,
    RENDER_FLAG_CULL = 1 << 2,
    RENDER_FLAG_BLEND = 1 << 3
} RenderFlags;

typedef struct RenderableComponent
{
    Model* model;
    uint32_t flag_mask;
} RenderableComponent;

typedef enum LightType
{
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_SPOT,
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_MAX
} LightType;

typedef struct PointLight
{
    vec3 diffuse;
    float quadratic;
    float linear;
} PointLight;

typedef struct SpotLight
{
    vec3 diffuse;
    float quadratic;
    float linear;
    float inner_cutoff; // cos of inner cutoff
    float outer_cutoff; // cos of outer cutoff
} SpotLight;

typedef struct DirectionalLight
{
    vec3 diffuse;
} DirectionalLight;

// diffuse should be a member of this struct but i'm an idiot and now everything breaks if i add it :)
typedef struct LightComponent
{
    LightType light_type;
    float intensity;
// this BARBARIC method of using unions should no longer be used due to the c11 switch but i'm too lazy to move the code
    union
    {
    PointLight point;
    SpotLight spot;
    DirectionalLight directional;
    } lights;
} LightComponent;

#define ENTITY_NAME_MAX_LENGTH 48
typedef struct Entity
{
    EntityID id;
    uint32_t component_mask;
    bool active;
    char name[ENTITY_NAME_MAX_LENGTH];
} Entity;

/*---- transform utilities ----*/
void transform_init(TransformComponent* t);
void transform_set_pos_3float(TransformComponent* t, float x, float y, float z);
void transform_set_pos_vec3(TransformComponent* t, vec3 xyz);
void transform_set_rotation_3float(TransformComponent* t, float x, float y, float z);
void transform_set_rotation_vec3(TransformComponent* t, vec3 xyz);
void transform_set_scale_3float(TransformComponent* t, float x, float y, float z);
void transform_set_scale_vec3(TransformComponent* t, vec3 xyz);
void transform_set_scale(TransformComponent* t, float scale);
void transform_update(TransformComponent* t);

/*---- renderables utilities ----*/
void renderable_init(RenderableComponent* r, Model* model);
void renderable_set_flag(RenderableComponent* r, RenderFlags f);
void renderable_clear_flag(RenderableComponent* r, RenderFlags f);
void renderable_toggle_flag(RenderableComponent* r, RenderFlags f);
bool renderable_has_flag(RenderableComponent* r, RenderFlags f);

/*---- light utilities ----*/
// these will start on default constants, declared and defined @ entity.c
LightComponent light_init_point(void);
LightComponent light_init_spot(void);
LightComponent light_init_directional(void);
// point light
void point_set_diffuse_3float(PointLight* t, float x, float y, float z);
void point_set_diffuse_vec3(PointLight* t, vec3 xyz);
void point_set_quadratic(PointLight* t, float q);
void point_set_linear(PointLight* t, float l);
// spot light
void spot_set_diffuse_3float(SpotLight* t, float x, float y, float z);
void spot_set_diffuse_vec3(SpotLight* t, vec3 xyz);
void spot_set_quadratic(SpotLight* t, float q);
void spot_set_linear(SpotLight* t, float l);
void spot_set_inner_cutoff(SpotLight* t, float deg);
void spot_set_outerr_cutoff(SpotLight* t, float deg);
// directional light
void directional_set_diffuse_3float(DirectionalLight* t, float x, float y, float z);
void directional_set_diffuse_vec3(DirectionalLight* t, vec3 xyz);

/*---- entity utilities ----*/
void entity_init(Entity* e, EntityID id, const char* name);
void entity_add_component(Entity* e, ComponentType type);
void entity_remove_component(Entity* e, ComponentType type);
bool entity_has_component(const Entity* e, ComponentType type);


#endif

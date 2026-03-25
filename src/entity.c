#include "entity.h"
#include <math.h>
#include <string.h>

#define DEFAULT_LINEAR    0.027f
#define DEFAULT_QUADRATIC 0.0028f

/*---- entities utilities ----*/
void transform_init(TransformComponent *t)
{
    glm_vec3_zero(t->position);
    glm_vec3_zero(t->rotation);
    glm_vec3_one(t->scale);
    glm_mat4_identity(t->world_matrix);
    t->dirty = false;
}

void transform_set_pos_3float(TransformComponent *t, float x, float y, float z)
{
    t->position[0] = x;
    t->position[1] = y;
    t->position[2] = z;
    t->dirty = true;
}
 
void transform_set_pos_vec3(TransformComponent *t, vec3 xyz)
{
    glm_vec3_copy(xyz, t->position);
    t->dirty = true;
}
 
void transform_set_rotation_3float(TransformComponent *t, float x, float y, float z)
{
    t->rotation[0] = x;
    t->rotation[1] = y;
    t->rotation[2] = z;
    t->dirty = true;
}
 
void transform_set_rotation_vec3(TransformComponent *t, vec3 xyz)
{
    glm_vec3_copy(xyz, t->rotation);
    t->dirty = true;
}
 
void transform_set_scale_3float(TransformComponent *t, float x, float y, float z)
{
    t->scale[0] = x;
    t->scale[1] = y;
    t->scale[2] = z;
    t->dirty = true;
}
 
void transform_set_scale_vec3(TransformComponent *t, vec3 xyz)
{
    glm_vec3_copy(xyz, t->scale);
    t->dirty = true;
}

void transform_set_scale(TransformComponent* t, float scale)
{
    t->scale[0] = scale;
    t->scale[1] = scale;
    t->scale[2] = scale;
    t->dirty = true;
}

 
void transform_update(TransformComponent *t)
{
    if (!t->dirty)
        return;
 
    glm_mat4_identity(t->world_matrix);
    glm_translate(t->world_matrix, t->position);
 
    // apply euler rotations in xyz order
    glm_rotate(t->world_matrix, glm_rad(t->rotation[0]), (vec3){1.0f, 0.0f, 0.0f});
    glm_rotate(t->world_matrix, glm_rad(t->rotation[1]), (vec3){0.0f, 1.0f, 0.0f});
    glm_rotate(t->world_matrix, glm_rad(t->rotation[2]), (vec3){0.0f, 0.0f, 1.0f});
 
    glm_scale(t->world_matrix, t->scale);
 
    t->dirty = false;
}

/*---- renderable utilities ----*/
void renderable_init(RenderableComponent *r, Model *model)
{
    r->model     = model;
    r->flag_mask = RENDER_FLAG_VISIBLE; /* visible by default */
}
 
void renderable_set_flag(RenderableComponent *r, RenderFlags f)
{
    r->flag_mask |= f;
}
 
void renderable_clear_flag(RenderableComponent *r, RenderFlags f)
{
    r->flag_mask &= ~f;
}
 
void renderable_toggle_flag(RenderableComponent *r, RenderFlags f)
{
    r->flag_mask ^= f;
}
 
bool renderable_has_flag(RenderableComponent *r, RenderFlags f)
{
    return (r->flag_mask & f) == (uint32_t)f;
}

/*---- light utilities ----*/
LightComponent light_init_point(void)
{
    LightComponent l;
    memset(&l, 0, sizeof(l));
 
    l.light_type = LIGHT_TYPE_POINT;
    l.intensity  = 1.0f;
 
    // default to white
    glm_vec3_one(l.lights.point.diffuse);
 
    l.lights.point.linear    = DEFAULT_LINEAR;
    l.lights.point.quadratic = DEFAULT_QUADRATIC;
 
    return l;
}
 
LightComponent light_init_spot(void)
{
    LightComponent l;
    memset(&l, 0, sizeof(l));
 
    l.light_type = LIGHT_TYPE_SPOT;
    l.intensity  = 150.0f;
 
    glm_vec3_one(l.lights.spot.diffuse);
 
    l.lights.spot.linear    = DEFAULT_LINEAR;
    l.lights.spot.quadratic = DEFAULT_QUADRATIC;
    
    // ALWAYS REMEMBER THAT THEY ARE STORED AS COS!!!!!!!!!
    l.lights.spot.inner_cutoff = cosf(glm_rad(15.0f));
    l.lights.spot.outer_cutoff = cosf(glm_rad(20.0f));
 
    return l;
}
 
LightComponent light_init_directional(void)
{
    LightComponent l;
    memset(&l, 0, sizeof(l));
 
    l.light_type = LIGHT_TYPE_DIRECTIONAL;
    l.intensity  = 1.0f;
 
    glm_vec3_one(l.lights.directional.diffuse);
 
    return l;
}

void shader_add_light(Shader* shader, LightComponent* lc, vec3 position, uint32_t index)
{
    char uniform_name[32];
    switch(lc->light_type)
    {
        case LIGHT_TYPE_POINT:
        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].position", index);
        shader_set_vec3(shader, uniform_name, position);

        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].constant", index);
        shader_set_float(shader, uniform_name, lc->lights.point.quadratic);
        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].linear", index);
        shader_set_float(shader, uniform_name, lc->lights.point.linear);

        snprintf(uniform_name, sizeof(uniform_name), "u_pointlights[%u].diffuse", index);
        shader_set_vec3(shader, uniform_name, lc->lights.point.diffuse);
        break;
        default:
        break;
    }
}

/*---- spotlight setters ----*/
 
void spot_set_diffuse_3float(SpotLight *t, float x, float y, float z)
{
    t->diffuse[0] = x;
    t->diffuse[1] = y;
    t->diffuse[2] = z;
}
 
void spot_set_diffuse_vec3(SpotLight *t, vec3 xyz)
{
    glm_vec3_copy(xyz, t->diffuse);
}
 
void spot_set_quadratic(SpotLight *t, float q)
{
    t->quadratic = q;
}

void spot_set_linear(SpotLight *t, float l)
{ 
    t->linear    = l;
}
 
// grab angles in degrees and convert to cosines to store
void spot_set_inner_cutoff(SpotLight *t, float deg)  
{ 
    t->inner_cutoff = cosf(glm_rad(deg));
}
void spot_set_outerr_cutoff(SpotLight *t, float deg) 
{ 
    t->outer_cutoff = cosf(glm_rad(deg));
}

/*---- point light setters ----*/
 
void point_set_diffuse_3float(PointLight *t, float x, float y, float z)
{
    t->diffuse[0] = x;
    t->diffuse[1] = y;
    t->diffuse[2] = z;
}
 
void point_set_diffuse_vec3(PointLight *t, vec3 xyz)
{
    glm_vec3_copy(xyz, t->diffuse);
}
 
void point_set_quadratic(PointLight *t, float q)
{
    t->quadratic = q;
}
void point_set_linear(PointLight *t, float l)
{
    t->linear = l;
}

/*---- directional light setters ----*/
void directional_set_diffuse_3float(DirectionalLight *t, float x, float y, float z)
{
    t->diffuse[0] = x;
    t->diffuse[1] = y;
    t->diffuse[2] = z;
}

void directional_set_diffuse_vec3(DirectionalLight *t, vec3 xyz)
{
    glm_vec3_copy(xyz, t->diffuse);
}

/*---- entities utilities ----*/
void entity_init(Entity *e, EntityID id, const char *name)
{
    e->id = id;
    e->component_mask = 0;
    strncpy(e->name, name, ENTITY_NAME_MAX_LENGTH - 1);
    e->name[ENTITY_NAME_MAX_LENGTH - 1] = '\0';
}
 
void entity_add_component(Entity *e, ComponentType type)
{
    e->component_mask |= type;
}
 
void entity_remove_component(Entity *e, ComponentType type)
{
    e->component_mask &= ~type;
}
 
bool entity_has_component(const Entity *e, ComponentType type)
{
    return (e->component_mask & type) == (uint32_t)type;
}
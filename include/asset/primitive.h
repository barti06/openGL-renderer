#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <stdbool.h>

#include "cglm/types.h"
#include "texture_loader.h"

typedef struct MaterialPBR
{
    GLuint albedo;
    GLuint metallic_roughness;

    vec4 albedo_factor;
    float metallic_factor;
    float roughness_factor;
} MaterialPBR;

typedef struct MaterialShared
{
    GLuint normal;
    GLuint emissive;
    GLuint ambient_occlusion;

    vec2 normal_scale;

    float occlusion_strength;
    vec2 occlusion_scale;
    bool has_occlusion_texcoord;

    vec3 emissive_factor;
    float emissive_strength;
    bool has_emissive_texcoord;
} MaterialShared;

typedef struct MaterialIridescence
{
    GLuint texture;
    GLuint thickness_texture;
    float factor;
    float ior;
    float thickness_min;
    float thickness_max;
} MaterialIridescence;

typedef struct MaterialSheen
{
    GLuint color_texture;
    GLuint roughness_texture;
    vec3 color_factor;
    float roughness_factor;
} MaterialSheen;

typedef struct MaterialClearcoat
{
    GLuint texture;
    GLuint roughness_texture;
    GLuint normal_texture;
    float factor;
    float roughness_factor;
} MaterialClearcoat;

typedef struct MaterialVolume
{
    GLuint  thickness_texture;
    float thickness_factor;
    float attenuation_distance;
    vec3 attenuation_color;
} MaterialVolume;

typedef struct 
{
    GLuint transmission_texture;
    float transmission_factor;
} MaterialTransmission;

typedef struct Material
{
    MaterialPBR pbr;

    MaterialShared shared;

    bool has_iridescence;
    MaterialIridescence iridescence;

    bool has_sheen;
    MaterialSheen sheen;

    bool has_clearcoat;
    MaterialClearcoat clearcoat;

    bool has_volume;
    MaterialVolume volume;

    bool has_transmission;
    MaterialTransmission transmission;

    bool double_sided;
    bool unlit;
} Material;

typedef struct Primitive
{
    // openGL handles
    GLuint VAO;
    GLuint VBO_positions;
    GLuint VBO_uvs;
    GLuint VBO_uvs2;
    GLuint VBO_normals;
    GLuint VBO_tangents;
    GLuint EBO;

    // num of instances to draw
    GLsizei index_count;
    GLenum  index_type;

    Material material;
} Primitive;

typedef struct Mesh
{
    mat4 transform;
    Primitive* primitives;
    uint32_t primitive_count;
    // 0 is min 1 is max
    vec3 aabb[2];
} Mesh;

int primitive_load(Primitive* model_primitive, cgltf_primitive* src, const char* base_path, TextureCache* cache);

void primitive_free(Primitive* model_primitive);

void mesh_free(Mesh* model_mesh);

#endif

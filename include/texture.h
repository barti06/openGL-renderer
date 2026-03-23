#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <cgltf.h>

#define MAX_MODEL_PATH_LENGTH 512

typedef struct TextureCacheEntry
{
    cgltf_image* image;
    GLuint texture_id;
} TextureCacheEntry;

typedef struct TextureCache
{
    TextureCacheEntry* entries;
    uint32_t count;
    uint32_t capacity;
} TextureCache;

GLuint texture_loader(cgltf_texture_view* texture_view, const char* base_path, TextureCache* cache);
// pbr-metallic-roughness textures
GLuint albedo_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache);
GLuint metallic_roughness_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache);
// iridescence textures
GLuint iridescence_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache);
GLuint iridescence_thickness_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache);

// other textures
GLuint normal_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache);
GLuint emissive_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache);
GLuint occlusion_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache);

void texture_cache_init(TextureCache* cache, uint32_t initial_capacity);
void texture_cache_free(TextureCache* cache);
// returns 0 if not found
GLuint texture_cache_get(const TextureCache* cache, cgltf_image* image);
// insert a completed tex_id for a given image pointer
void texture_cache_put(TextureCache* cache, cgltf_image* image, GLuint tex_id);
#endif
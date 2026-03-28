#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <glad/glad.h>
#include <cgltf.h>

#define MAX_MODEL_PATH_LENGTH 256

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

typedef struct
{
    cgltf_image* image;
    const char* base_path;
    unsigned char* pixels;
    int width;
    int height;
    int channels;
    int failed;
} TextureDecodeJob;

void texture_decode_all(cgltf_data* data, const char* base_path, TextureCache* cache);

GLuint load_texture_view(cgltf_texture_view* texture_view, const char* base_path, TextureCache* cache);

// for faster texture loading
void texture_cache_init(TextureCache* cache, uint32_t initial_capacity);
void texture_cache_free(TextureCache* cache);

// returns 0 if not found
GLuint texture_cache_get(const TextureCache* cache, cgltf_image* image);
#endif

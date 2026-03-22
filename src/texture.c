#include "texture.h"
#include "stb_image.h"
#include <log.h>
#include <stdlib.h>
#include <string.h>

GLuint texture_loader(cgltf_texture_view* texture_view, const char* base_path, TextureCache* cache)
{
    cgltf_image* image = texture_view->texture->image;
    
    uint32_t cached_tex = texture_cache_get(cache, image);
    if(cached_tex)
        return cached_tex;

    int width, height, channels;
    unsigned char *pixels = NULL;
    if (image->buffer_view) 
    {
        // unpack texture embedded into .glb
        uint8_t* data = (uint8_t*)image->buffer_view->buffer->data + image->buffer_view->offset;
        pixels = stbi_load_from_memory(data, (int)image->buffer_view->size, &width, &height, &channels, 0);
    } 
    else if (image->uri) 
    {
        // get texture from an external folder
        char full_path[MAX_MODEL_PATH_LENGTH * 2];
        snprintf(full_path, sizeof(full_path), "%s%s", base_path, image->uri);
        pixels = stbi_load(full_path, &width, &height, &channels, 0);
    }

    if (!pixels) 
    {
        log_error("ERROR... albedo_texture_load() SAYS: STBI FAILED! REASON: %s", stbi_failure_reason());
        return 0;
    }

    GLenum internal_format, external_format;
    int alignment;

    switch (channels) 
    {
    case 1:
        internal_format = GL_R8;
        external_format = GL_RED;
        alignment = 1;
        break;
    case 2:
        internal_format = GL_RG8;
        external_format = GL_RG;
        alignment = 2;
        break;
    case 3:
        internal_format = GL_RGB8;
        external_format = GL_RGB;
        alignment = 1;
        break;
    case 4:
        internal_format = GL_RGBA8;
        external_format = GL_RGBA;
        alignment = 4;
        break;
    default:
        log_error("ERROR... albedo_texture_load() SAYS: UNEXPECTED ALBEDO TEXTURE CHANNEL COUNT = %d!", channels);
        stbi_image_free(pixels);
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // modify alignment, just in case a texture has a different format than GL_RGBA
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, external_format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    // use gltf texture sampling options, otherwise resort to defaults
    cgltf_sampler* sampler = texture_view->texture->sampler;
    int min_filter = (sampler && sampler->min_filter) ? (int)sampler->min_filter : GL_LINEAR_MIPMAP_LINEAR;
    int mag_filter = (sampler && sampler->mag_filter) ? (int)sampler->mag_filter : GL_LINEAR;
    int wrap_s = (sampler && sampler->wrap_s) ? (int)sampler->wrap_s : GL_REPEAT;
    int wrap_t = (sampler && sampler->wrap_t) ? (int)sampler->wrap_t : GL_REPEAT;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);

    // setup anisotropic filtering
    GLfloat max_anisotropic;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropic);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, max_anisotropic);

    stbi_image_free(pixels);

    texture_cache_put(cache, image, tex);

    return tex;
}

/*----PBR METALLIC ROUGHNESS FUNCTIONS----*/
GLuint albedo_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache)
{
    // get albedo texture handle from the model primitive
    cgltf_texture_view* texture_view = &primitive->material->pbr_metallic_roughness.base_color_texture;

    if (!texture_view->texture || !texture_view->texture->image) 
    {
        log_error("ERROR... albedo_texture_load() SAYS: NO ALBEDO TEXTURE ON GIVEN PRIMITIVE");
        return 0;
    }

    return texture_loader(texture_view, base_path, cache);
}
GLuint metallic_roughness_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache)
{
    // get metallic roughness texture handle from the model primitive
    cgltf_texture_view* texture_view = &primitive->material->pbr_metallic_roughness.metallic_roughness_texture;
    if (!texture_view->texture || !texture_view->texture->image) 
    {
        log_error("ERROR... metallic_roughness_texture_load() SAYS: NO METALLIC ROUGHNESS TEXTURE ON GIVEN PRIMITIVE");
        return 0;
    }

    return texture_loader(texture_view, base_path, cache);
}

/*---- other textures ----*/
GLuint normal_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache)
{
    // get normal texture handle from the model primitive
    cgltf_texture_view* texture_view = &primitive->material->normal_texture;
    if (!texture_view->texture || !texture_view->texture->image) 
    {
        //log_error("ERROR... normal_texture_load() SAYS: NO NORMAL TEXTURE ON GIVEN PRIMITIVE");
        return 0;
    }

    return texture_loader(texture_view, base_path, cache);
}

GLuint emissive_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache)
{
    // get emissive texture handle from the model primitive
    cgltf_texture_view* texture_view = &primitive->material->emissive_texture;
    if (!texture_view->texture || !texture_view->texture->image) 
    {
        //log_error("ERROR... emissive_texture_load() SAYS: NO EMISSIVE TEXTURE ON GIVEN PRIMITIVE");
        return 0;
    }

    return texture_loader(texture_view, base_path, cache);
}

GLuint occlusion_texture_load(cgltf_primitive* primitive, const char* base_path, TextureCache* cache)
{
    // get emissive texture handle from the model primitive
    cgltf_texture_view* texture_view = &primitive->material->occlusion_texture;
    if (!texture_view->texture || !texture_view->texture->image) 
    {
        //log_error("ERROR... ambient_texture_load() SAYS: NO OCCLUSION TEXTURE ON GIVEN PRIMITIVE");
        return 0;
    }

    return texture_loader(texture_view, base_path, cache);
}

/*---- texture cache funcions definitions ----*/
void texture_cache_init(TextureCache *cache, uint32_t initial_capacity)
{
    cache->entries = calloc(initial_capacity, sizeof(TextureCacheEntry));
    cache->count = 0;
    cache->capacity = initial_capacity;
}

void texture_cache_free(TextureCache *cache)
{
    free(cache->entries);
    memset(cache, 0, sizeof(*cache));
}

GLuint texture_cache_get(const TextureCache *cache, cgltf_image *image)
{
    for(uint32_t index = 0; index < cache->count; index++)
    {
        if(cache->entries[index].image == image)
            return cache->entries[index].texture_id;
    }
    return 0;
}

void texture_cache_put(TextureCache *cache, cgltf_image *image, GLuint tex_id)
{
    if(cache->count == cache->capacity)
    {
        cache->capacity *= 2;
        cache->entries = realloc(cache->entries, cache->capacity * sizeof(TextureCache));
    }
    cache->entries[cache->count++] = (TextureCacheEntry) {image, tex_id};
}
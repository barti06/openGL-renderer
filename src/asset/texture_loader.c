#include "texture_loader.h"
#include "stb_image.h"
#include <log.h>
#include <string.h>
#include <threads.h>

// insert a completed tex_id for a given image pointer
static inline void texture_cache_put(TextureCache* cache, cgltf_image* image, GLuint tex_id);

// the actual image decoder
static inline int decode_job_fn(void* arg);

// main thread data uploader
static inline GLuint gl_upload_decoded(TextureDecodeJob* job, cgltf_texture_view* view);

// decodes every image in parallel, must be called from model_load()
void texture_decode_all(cgltf_data* data, const char* base_path, TextureCache* cache)
{
    uint32_t count = (uint32_t)data->images_count;
    if (count == 0) 
        return;

    TextureDecodeJob* jobs = calloc(count, sizeof(TextureDecodeJob));
    thrd_t* threads = calloc(count, sizeof(thrd_t));

    if (!jobs || !threads)
    {
        log_error("ERROR... texture_decode_all() SAYS: OUT OF MEMORY");
        free(jobs); free(threads);
        return;
    }

    // setup a decode thread per image
    for (uint32_t i = 0; i < count; i++)
    {
        jobs[i].image = &data->images[i];
        jobs[i].base_path = base_path;
        thrd_create(&threads[i], decode_job_fn, &jobs[i]);
    }

    // wait for all decodes & upload on the main thread
    for (uint32_t i = 0; i < count; i++)
    {
        thrd_join(threads[i], NULL);

        if (jobs[i].failed || !jobs[i].pixels)
        {
            log_error("ERROR... texture_decode_all() SAYS: FAILED TO DECODE IMAGE %u", i);
            continue;
        }

        // find a texture_view that references this image to get sampler params
        // just use the first material that references it
        cgltf_texture_view* ref_view = NULL;
        for (size_t mi = 0; mi < data->materials_count && !ref_view; mi++)
        {
            cgltf_material* mat = &data->materials[mi];
            if (mat->has_pbr_metallic_roughness)
            {
                if (mat->pbr_metallic_roughness.base_color_texture.texture &&
                    mat->pbr_metallic_roughness.base_color_texture.texture->image == jobs[i].image)
                    ref_view = &mat->pbr_metallic_roughness.base_color_texture;
                else if (mat->pbr_metallic_roughness.metallic_roughness_texture.texture &&
                         mat->pbr_metallic_roughness.metallic_roughness_texture.texture->image == jobs[i].image)
                    ref_view = &mat->pbr_metallic_roughness.metallic_roughness_texture;
            }
        }

        GLuint tex = gl_upload_decoded(&jobs[i], ref_view);
        if (tex)
            texture_cache_put(cache, jobs[i].image, tex);

        stbi_image_free(jobs[i].pixels);
    }

    free(jobs);
    free(threads);
}

GLuint load_texture_view(cgltf_texture_view* texture_view, const char* base_path, TextureCache* cache)
{
    if(!texture_view->texture)
    {
        //log_error("ERROR... load_texture_view() SAYS: NO TEXTURE IN TEXTURE VIEW!");
        return 0;
    }
    cgltf_image* image = texture_view->texture->image;

    if(!image)
    {
        log_error("ERROR... load_texture_view() SAYS: COULDN'T FIND IMAGE IN TEXTURE!");
        return 0;
    }
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
        cache->entries = realloc(cache->entries, cache->capacity * sizeof(TextureCacheEntry));
    }
    cache->entries[cache->count++] = (TextureCacheEntry) {image, tex_id};
}

static inline int decode_job_fn(void* arg)
{
    TextureDecodeJob* job = (TextureDecodeJob*)arg;

    if (job->image->buffer_view)
    {
        uint8_t* data = (uint8_t*)job->image->buffer_view->buffer->data + job->image->buffer_view->offset;
        job->pixels = stbi_load_from_memory(data, (int)job->image->buffer_view->size, &job->width, &job->height, &job->channels, 0);
    }
    else if (job->image->uri)
    {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s%s", job->base_path, job->image->uri);
        job->pixels = stbi_load(full_path, &job->width, &job->height, &job->channels, 0);
    }

    if (!job->pixels)
        job->failed = 1;

    return 0;
}

static inline GLuint gl_upload_decoded(TextureDecodeJob* job, cgltf_texture_view* view)
{
    GLenum internal_format, external_format;
    int alignment;

    switch (job->channels)
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
        external_format = GL_RGB; alignment = 1; 
        break;

        case 4:
        internal_format = GL_RGBA8;
        external_format = GL_RGBA;
        alignment = 4;
        break;
        default:
            log_error("ERROR... gl_upload_decoded() SAYS: UNEXPECTED CHANNEL COUNT %d", job->channels);
            return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, job->width, job->height, 0, external_format, GL_UNSIGNED_BYTE, job->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    cgltf_sampler* sampler = view ? view->texture->sampler : NULL;
    int min_filter = (sampler && sampler->min_filter) ? sampler->min_filter : GL_LINEAR_MIPMAP_LINEAR;
    int mag_filter = (sampler && sampler->mag_filter) ? sampler->mag_filter : GL_LINEAR;
    int wrap_s = (sampler && sampler->wrap_s) ? sampler->wrap_s : GL_REPEAT;
    int wrap_t = (sampler && sampler->wrap_t) ? sampler->wrap_t : GL_REPEAT;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);

    GLfloat max_aniso;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);

    return tex;
}

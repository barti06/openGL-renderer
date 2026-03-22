#include <primitive.h>
#include <log.h>
#include <string.h>

int primitive_load(Primitive* model_primitive, cgltf_primitive *src, const char *base_path)
{
    memset(model_primitive, 0, sizeof(*model_primitive));
 
    glGenVertexArrays(1, &model_primitive->VAO);
    glBindVertexArray(model_primitive->VAO);
 
    glGenBuffers(1, &model_primitive->VBO_positions);
    glGenBuffers(1, &model_primitive->VBO_normals);
    glGenBuffers(1, &model_primitive->VBO_uvs);
    glGenBuffers(1, &model_primitive->EBO);
 
    // flag to check correct gpu data upload
    int ok = 1;
 
    // upload data to gpu
    for (uint64_t attribute_index = 0; attribute_index < src->attributes_count; attribute_index++)
    {
        cgltf_attribute* attribute = &src->attributes[attribute_index];
        
        // pass primitive positions to gpu
        if(attribute->type == cgltf_attribute_type_position)
            ok &= upload_accessor_float(attribute->data, model_primitive->VBO_positions, 0, 3);
        // pass primitive normals to gpu
        else if(attribute->type == cgltf_attribute_type_normal)
            ok &= upload_accessor_float(attribute->data, model_primitive->VBO_normals, 1, 3);
        // pass primitive uvs to gpu
        else if(attribute->type == cgltf_attribute_type_texcoord && attribute->index == 0)
            ok &= upload_accessor_float(attribute->data, model_primitive->VBO_uvs, 2, 2);
        else if(attribute->type == cgltf_attribute_type_tangent)
            ok &= upload_accessor_float(attribute->data, model_primitive->VBO_tangents, 3, 4);
    }
 
    if (src->indices)
    {
        ok &= upload_accessor_index(src->indices, model_primitive->EBO, &model_primitive->index_count);
        model_primitive->index_type = GL_UNSIGNED_INT;
    }
 
    glBindVertexArray(0);
 
    if (!ok) 
        return 0;
 
    if (!src->material) 
    {
        log_error("ERROR... primitive_load() SAYS: PRIMITIVE CONTAINS NO MATERIAL");
        return 0;
    }

    if(src->material->has_pbr_metallic_roughness)
    {
        model_primitive->albedo = albedo_texture_load(src, base_path);
        model_primitive->metallic_roughness = metallic_roughness_texture_load(src, base_path);
        // wont add support for albedo factor! :D
        model_primitive->metallic_factor = src->material->pbr_metallic_roughness.metallic_factor;
        model_primitive->roughness_factor = src->material->pbr_metallic_roughness.roughness_factor;
    }
    model_primitive->normal = normal_texture_load(src, base_path);

    return 1;
}
 
void primitive_free(Primitive* model_primitive)
{
    if (!model_primitive) 
        return;

    if (model_primitive->VAO)           
        glDeleteVertexArrays(1, &model_primitive->VAO);
    if (model_primitive->VBO_positions) 
        glDeleteBuffers(1, &model_primitive->VBO_positions);
    if (model_primitive->VBO_normals)   
        glDeleteBuffers(1, &model_primitive->VBO_normals);
    if (model_primitive->VBO_uvs)       
        glDeleteBuffers(1, &model_primitive->VBO_uvs);
    if (model_primitive->EBO)           
        glDeleteBuffers(1, &model_primitive->EBO);
    if (model_primitive->albedo)        
        glDeleteTextures(1, &model_primitive->albedo);

    memset(model_primitive, 0, sizeof(*model_primitive));
}

/*----START ACCESSOR DEFINITIONS----*/

int upload_accessor_float(cgltf_accessor *accessor, GLuint VBO, GLuint attrib_index, GLint num_components)
{
    if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer->data) 
    {
        log_error("ERROR... upload_float_accessor SAYS: ACCESSOR DATA COULD NOT LOAD.");
        return 0;
    }

    uint64_t float_count = accessor->count * (uint64_t)num_components;
    float* buf = (float*)malloc(float_count * sizeof(float));
    if (!buf)  
        return 0;

    cgltf_accessor_unpack_floats(accessor, buf, float_count);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(float_count * sizeof(float)), buf, GL_STATIC_DRAW);
    glEnableVertexAttribArray(attrib_index);
    glVertexAttribPointer(attrib_index, num_components, GL_FLOAT, GL_FALSE, 0, (void *)0);

    free(buf);
    
    return 1;
}

int upload_accessor_index(cgltf_accessor *accessor, GLuint EBO, GLsizei *out_count)
{
    if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer->data) 
    {
        log_error("ERROR... upload_index_accessor SAYS: INDEX ACCESSOR DATA COULD NOT LOAD!");
        return 0;
    }

    uint32_t* indices = (uint32_t*)malloc(accessor->count * sizeof(uint32_t));
    if(!indices) 
        return 0;

    for(size_t accessor_index = 0; accessor_index < accessor->count; accessor_index++) 
        cgltf_accessor_read_uint(accessor, accessor_index, &indices[accessor_index], 1);


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
        (GLsizeiptr)(accessor->count * sizeof(uint32_t)), indices, GL_STATIC_DRAW);

    *out_count = (GLsizei)accessor->count;

    free(indices);

    return 1;
}

/*----START TEXTURES DEFINITIONS----*/

static GLuint texture_loader(cgltf_texture_view* texture_view, const char* base_path)
{
    cgltf_image* image = texture_view->texture->image;

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
    int wrap_s     = (sampler && sampler->wrap_s)     ? (int)sampler->wrap_s     : GL_REPEAT;
    int wrap_t     = (sampler && sampler->wrap_t)     ? (int)sampler->wrap_t     : GL_REPEAT;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     wrap_t);

    // setup anisotropic filtering
    GLfloat max_anisotropic;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropic);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, max_anisotropic);

    stbi_image_free(pixels);
    return tex;
}

GLuint albedo_texture_load(cgltf_primitive* primitive, const char* base_path)
{
    // get albedo texture handle from the model primitive
    cgltf_texture_view* texture_view = &primitive->material->pbr_metallic_roughness.base_color_texture;

    if (!texture_view->texture || !texture_view->texture->image) 
    {
        log_error("ERROR... albedo_texture_load() SAYS: NO ALBEDO TEXTURE ON GIVEN PRIMITIVE");
        return 0;
    }

    return texture_loader(texture_view, base_path);
}

GLuint metallic_roughness_texture_load(cgltf_primitive* primitive, const char* base_path)
{
    // get albedo texture handle from the model primitive
    cgltf_texture_view* texture_view = &primitive->material->pbr_metallic_roughness.metallic_roughness_texture;
    if (!texture_view->texture || !texture_view->texture->image) 
    {
        log_error("ERROR... metallic_roughness_texture_load() SAYS: NO METALLIC ROUGHNESS TEXTURE ON GIVEN PRIMITIVE");
        return 0;
    }

    return texture_loader(texture_view, base_path);
}

GLuint normal_texture_load(cgltf_primitive* primitive, const char* base_path)
{
    // get albedo texture handle from the model primitive
    cgltf_texture_view* texture_view = &primitive->material->normal_texture;
    if (!texture_view->texture || !texture_view->texture->image) 
    {
        log_error("ERROR... normal_texture_load() SAYS: NO NORMAL TEXTURE ON GIVEN PRIMITIVE");
        return 0;
    }

    return texture_loader(texture_view, base_path);
}
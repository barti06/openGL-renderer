#include "shader.h"
#include "model.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int upload_accessor_float(cgltf_accessor *accessor, GLuint VBO, GLuint attrib_index, GLint num_components);

static int upload_accessor_index(cgltf_accessor *accessor, GLuint EBO, GLsizei *out_count);

static GLuint albedo_texture_load(cgltf_primitive *model_primitive, const char *base_path);

int model_load(Model* model, const char* location)
{
    if(!model || !location)
        return 0;

    // set all memory to 0
    memset(model, 0, sizeof(*model));
    strncpy(model->filepath, location, MAX_MODEL_PATH_LENGTH - 1);
    model->filepath[MAX_MODEL_PATH_LENGTH - 1] = '\0';

    model->is_loaded = 0;

    cgltf_options model_options = {0};
    cgltf_data* model_data = NULL;

    // parse the file 
    cgltf_result result = cgltf_parse_file(&model_options, location, &model_data);
    if(result != cgltf_result_success)
    {
        log_error("ERROR... model_load() SAYS: cgltf_parse_file FAILED. RESULT = %d", result);
        return 0;
    }

    // load associated buffers
    result = cgltf_load_buffers(&model_options, model_data, location);
    if(result != cgltf_result_success)
    {
        log_error("ERROR... model_load() SAYS: cgltf_load_buffers FAILED. RESULT = %d", result);
        return 0;
    }

    if(model_data->meshes_count == 0)
    {
        log_error("ERROR... void model_load(Model* model, const char* location) SAYS: NO MESHES IN FILE.");
        cgltf_free(model_data);
        return 0;
    }

    // get base directory for external texture loading
    char base_path[256] = {0};
    const char* last_sep = strrchr(location, '/');
    if (!last_sep) 
        last_sep = strrchr(location, '\\');
    if (last_sep) 
    {
        uint64_t len = (uint64_t)(last_sep - location) + 1;
        if (len < sizeof(base_path))
        {
            memcpy(base_path, location, len);
            base_path[len] = '\0';
        }
    }

    // generate buffers
    glGenVertexArrays(1, &model->VAO);
    glBindVertexArray(model->VAO);
    glGenBuffers(1, &model->VBO_positions);
    glGenBuffers(1, &model->VBO_normals);
    glGenBuffers(1, &model->VBO_uvs);
    glGenBuffers(1, &model->EBO);

    int geometry_success = 1;

    cgltf_primitive* model_primitive = &model_data->meshes[0].primitives[0];

    // upload vertex attributes
    for (size_t array_index = 0; array_index < model_primitive->attributes_count; array_index++) 
    {
        cgltf_attribute* attribute = &model_primitive->attributes[array_index];

        if (attribute->type == cgltf_attribute_type_position) 
        {
            geometry_success &= upload_accessor_float(attribute->data, 
                model->VBO_positions, 0, 3);
        } 
        else if (attribute->type == cgltf_attribute_type_normal) 
        {
            geometry_success &= upload_accessor_float(attribute->data, 
                model->VBO_normals, 1, 3);
        } 
        else if (attribute->type == cgltf_attribute_type_texcoord && attribute->index == 0) 
        {
            geometry_success &= upload_accessor_float(attribute->data, 
                model->VBO_uvs, 2, 2);
        }
    }

    // upload indices
    if(model_primitive->indices)
    {
        geometry_success &= upload_accessor_index(model_primitive->indices, 
            model->EBO, &model->index_count);
        
        model->index_type = GL_UNSIGNED_INT;
    }

    glBindVertexArray(0);

    if(!geometry_success)
    {
        log_error("ERROR... void model_load(Model* model, const char* location) SAYS: FAILED TO LOAD GEOMETRY.");
        model_free(model);
        cgltf_free(model_data);
        return 0;
    }

    // load textures (only albedo for now because im lazy :D)
    model->albedo = albedo_texture_load(model_primitive, base_path);
    if(!model->albedo)
        log_error("WARNING!!! void model_load(Model* model, const char* location) SAYS: FAILED TO LOAD ALBEDO TEXTURES.");
    
    cgltf_free(model_data);

    model->is_loaded = 1;

    return 1;
}

void model_draw(const Model* model, Shader* shader)
{
    // activate the shader
    shader_use(shader);

    // send albedo texture, only if model has one
    if(model->albedo)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model->albedo);
        shader_set_int(shader, "u_albedo", 0);
    }

    // bind vao
    glBindVertexArray(model->VAO);

    // draw the model
    glDrawElements(GL_TRIANGLES, model->index_count,
        model->index_type, (void* )0);
    
    // unbind vao
    glBindVertexArray(0);

}

void model_free(Model *model)
{
    if (!model) return;

    // delete openGL buffers
    if(model->VAO)
        glDeleteVertexArrays(1, &model->VAO);
    if(model->VBO_positions)
        glDeleteBuffers(1, &model->VBO_positions);
    if(model->VBO_normals) 
        glDeleteBuffers(1, &model->VBO_normals);
    if(model->VBO_uvs)
        glDeleteBuffers(1, &model->VBO_uvs);
    if(model->EBO)
        glDeleteBuffers(1, &model->EBO);
    
    // delete textures
    if (model->albedo) 
        glDeleteTextures(1, &model->albedo);

    // reset struct
    memset(model, 0, sizeof(*model));
}

/*--INTERNAL FUNCTIONS START--*/

GLuint albedo_texture_load(cgltf_primitive *model_primitive, const char *base_path)
{
    if (!model_primitive->material) 
    {
        log_error("ERROR... load_albedo_texture() SAYS: PRIMITIVE CONTAINS NO MATERIAL");
        return 0;
    }

    // get the texture handle from the model primitive
    cgltf_texture_view* texture_view = &model_primitive->material->pbr_metallic_roughness.base_color_texture;

    if (!texture_view->texture || !texture_view->texture->image) 
    {
        log_error("ERROR... load_albedo_texture() SAYS: NO MATERIAL ON GIVEN PRIMITIVE");
        return 0;
        
    }

    cgltf_image* image = texture_view->texture->image;

    int width, height, channels;
    unsigned char *pixels = NULL;
    if (image->buffer_view) 
    {
        // unpack image embedded into .glb
        uint8_t* data = (uint8_t*)image->buffer_view->buffer->data + image->buffer_view->offset;
        int data_len  = (int)image->buffer_view->size;
        pixels = stbi_load_from_memory(data, data_len, &width, &height, &channels, 4);
    } 
    else if (image->uri) 
    {
        // unpack image from an external folder
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s%s", base_path, image->uri);
        pixels = stbi_load(full_path, &width, &height, &channels, 4);
    }

    if (!pixels) 
    {
        log_error("ERROR... albedo_texture_load() SAYS: STBI FAILED! REASON: %s", stbi_failure_reason());
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // MUST send RGBA8 because it's stbi image was forced to 4 channels
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
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

    stbi_image_free(pixels);
    return tex;
}

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

    uint32_t *indices = (uint32_t*)malloc(accessor->count * sizeof(uint32_t));
    if (!indices) return 0;

    for (size_t i = 0; i < accessor->count; i++) 
        cgltf_accessor_read_uint(accessor, i, &indices[i], 1);


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
        (GLsizeiptr)(accessor->count * sizeof(uint32_t)), indices, GL_STATIC_DRAW);

    *out_count = (GLsizei)accessor->count;

    free(indices);

    return 1;
}
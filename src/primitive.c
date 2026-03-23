#include <primitive.h>
#include <log.h>
#include <string.h>
#include <malloc.h>

int primitive_load(Primitive* model_primitive, cgltf_primitive *src, const char *base_path, TextureCache* cache)
{
    memset(model_primitive, 0, sizeof(*model_primitive));
 
    /*---- GPU DATA SEND START ----*/

    // flag to check correct gpu data upload
    int ok = 1;
    glGenVertexArrays(1, &model_primitive->VAO);
    glBindVertexArray(model_primitive->VAO);
 
    glGenBuffers(1, &model_primitive->VBO_positions);
    glGenBuffers(1, &model_primitive->VBO_normals);
    glGenBuffers(1, &model_primitive->VBO_uvs);
    glGenBuffers(1, &model_primitive->VBO_tangents);
    glGenBuffers(1, &model_primitive->EBO);
    
    // upload attribute data to gpu
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
        // pass primitive tangents to gpu
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
    /*---- GPU DATA SEND END ----*/

    if (!src->material) 
    {
        log_error("ERROR... primitive_load() SAYS: PRIMITIVE CONTAINS NO MATERIAL");
        return 0;
    }

    if(src->material->has_pbr_metallic_roughness)
    {  
        // textures
        model_primitive->albedo = albedo_texture_load(src, base_path, cache);
        model_primitive->metallic_roughness = metallic_roughness_texture_load(src, base_path, cache);

        model_primitive->albedo_factor[0] = src->material->pbr_metallic_roughness.base_color_factor[0];
        model_primitive->albedo_factor[1] = src->material->pbr_metallic_roughness.base_color_factor[1];
        model_primitive->albedo_factor[2] = src->material->pbr_metallic_roughness.base_color_factor[2];
        model_primitive->albedo_factor[3] = src->material->pbr_metallic_roughness.base_color_factor[3];

        model_primitive->metallic_factor = src->material->pbr_metallic_roughness.metallic_factor;
        model_primitive->roughness_factor = src->material->pbr_metallic_roughness.roughness_factor;
    }

    if(src->material->has_emissive_strength)
        model_primitive->emissive_strength = src->material->emissive_strength.emissive_strength;
    else
        model_primitive->emissive_strength = 1.0f;

    if(src->material->has_iridescence)
    {
        model_primitive->iridescence = iridescence_texture_load(src, base_path, cache);
        model_primitive->iridescence_thickness = iridescence_thickness_texture_load(src, base_path, cache);
        model_primitive->iridescence_thickness_max = src->material->iridescence.iridescence_thickness_max;
        model_primitive->iridescence_thickness_min = src->material->iridescence.iridescence_thickness_min;
        model_primitive->iridescence_factor = src->material->iridescence.iridescence_factor;
        model_primitive->iridescence_ior = src->material->iridescence.iridescence_ior;
    }
    // get primitive emissive texture
    model_primitive->emissive = emissive_texture_load(src, base_path, cache);
    memcpy(model_primitive->emissive_factor, src->material->emissive_factor, sizeof(vec3));

    // get primitive ao texture 
    model_primitive->ambient_occlusion = occlusion_texture_load(src, base_path, cache);
    memcpy(model_primitive->occlusion_scale, src->material->occlusion_texture.transform.scale, sizeof(vec2));

    // get primitive normal texture
    model_primitive->normal = normal_texture_load(src, base_path, cache);
    memcpy(model_primitive->normal_scale, src->material->normal_texture.transform.scale, sizeof(vec2));

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
    if (model_primitive->metallic_roughness)        
        glDeleteTextures(1, &model_primitive->metallic_roughness);
    if (model_primitive->normal)        
        glDeleteTextures(1, &model_primitive->normal);
    if (model_primitive->emissive)        
        glDeleteTextures(1, &model_primitive->emissive);
    if (model_primitive->ambient_occlusion)        
        glDeleteTextures(1, &model_primitive->ambient_occlusion);

    memset(model_primitive, 0, sizeof(*model_primitive));
}

void mesh_free(Mesh* model_mesh)
{
    if(!model_mesh)
        return;

    // free each primitive
    for(size_t i = 0; i < model_mesh->primitive_count; i++)
        primitive_free(&model_mesh->primitives[i]);

    // deallocate primitive's memory 
    free(model_mesh->primitives);

    memset(model_mesh, 0, sizeof(*model_mesh));
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
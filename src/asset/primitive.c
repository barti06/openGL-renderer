#include "primitive.h"
#include <log.h>
#include <string.h>
#include <malloc.h>

static inline void material_load(Material* mat, cgltf_primitive* src, const char* base_path, TextureCache* cache);

static inline int upload_accessor_float(cgltf_accessor* accessor, GLuint VBO, GLuint attrib_index, GLint num_components);

static inline int upload_accessor_index(cgltf_accessor* accessor, GLuint EBO, GLsizei* out_count);

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
    glGenBuffers(1, &model_primitive->VBO_uvs2);
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
        else if(attribute->type == cgltf_attribute_type_texcoord && attribute->index == 1)
            ok &= upload_accessor_float(attribute->data, model_primitive->VBO_uvs2, 4, 2);
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

    material_load(&model_primitive->material, src, base_path, cache);
        
    return 1;
}

void material_free(Material* mat)
{
    // core
    if (mat->pbr.albedo)
        glDeleteTextures(1, &mat->pbr.albedo);
    if (mat->pbr.metallic_roughness)
        glDeleteTextures(1, &mat->pbr.metallic_roughness);
    // shared
    if (mat->shared.normal)
        glDeleteTextures(1, &mat->shared.normal);
    if (mat->shared.emissive)
        glDeleteTextures(1, &mat->shared.emissive);
    if (mat->shared.ambient_occlusion)
        glDeleteTextures(1, &mat->shared.ambient_occlusion);
    // extensions
    if (mat->has_iridescence) 
    {
        if (mat->iridescence.texture)
            glDeleteTextures(1, &mat->iridescence.texture);
        if (mat->iridescence.thickness_texture)
            glDeleteTextures(1, &mat->iridescence.thickness_texture);
    }
    if (mat->has_sheen) 
    {
        if (mat->sheen.color_texture)
            glDeleteTextures(1, &mat->sheen.color_texture);
        if (mat->sheen.roughness_texture)
            glDeleteTextures(1, &mat->sheen.roughness_texture);
    }
    if (mat->has_clearcoat) 
    {
        if (mat->clearcoat.texture)
            glDeleteTextures(1, &mat->clearcoat.texture);
        if (mat->clearcoat.roughness_texture)
            glDeleteTextures(1, &mat->clearcoat.roughness_texture);
        if (mat->clearcoat.normal_texture)
            glDeleteTextures(1, &mat->clearcoat.normal_texture);
    }
    if (mat->has_volume && mat->volume.thickness_texture)
        glDeleteTextures(1, &mat->volume.thickness_texture);
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
    if (model_primitive->VBO_uvs2)       
        glDeleteBuffers(1, &model_primitive->VBO_uvs2);
    if (model_primitive->EBO)           
        glDeleteBuffers(1, &model_primitive->EBO);
    
    material_free(&model_primitive->material);

    memset(model_primitive, 0, sizeof(*model_primitive));
}

/*----START ACCESSOR DEFINITIONS----*/

static inline int upload_accessor_float(cgltf_accessor *accessor, GLuint VBO, GLuint attrib_index, GLint num_components)
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

static inline int upload_accessor_index(cgltf_accessor *accessor, GLuint EBO, GLsizei *out_count)
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

static inline void material_load(Material* mat, cgltf_primitive* src, const char* base_path, TextureCache* cache)
{
    memset(mat, 0, sizeof(*mat));

    cgltf_material* m = src->material;
    if (!m)
        return;

    // load core PBR
    if (m->has_pbr_metallic_roughness)
    {
        mat->pbr.albedo = load_texture_view(&m->pbr_metallic_roughness.base_color_texture, base_path, cache);
        mat->pbr.metallic_roughness = load_texture_view(&m->pbr_metallic_roughness.metallic_roughness_texture, base_path, cache);
        memcpy(mat->pbr.albedo_factor, m->pbr_metallic_roughness.base_color_factor, sizeof(vec4));
        mat->pbr.metallic_factor = m->pbr_metallic_roughness.metallic_factor;
        mat->pbr.roughness_factor = m->pbr_metallic_roughness.roughness_factor;
    }

    // load normal maps
    mat->shared.normal = load_texture_view(&m->normal_texture, base_path, cache);
    memcpy(mat->shared.normal_scale, m->normal_texture.transform.scale, sizeof(vec2));

    // load emissive maps
    mat->shared.emissive = load_texture_view(&m->emissive_texture, base_path, cache);
    mat->shared.emissive_strength = m->has_emissive_strength ? m->emissive_strength.emissive_strength : 1.0f;
    if(m->emissive_texture.texcoord)
        mat->shared.has_emissive_texcoord = true;
    memcpy(mat->shared.emissive_factor, m->emissive_factor, sizeof(vec3));

    // load ao
    mat->shared.ambient_occlusion = load_texture_view(&m->occlusion_texture, base_path, cache);    
    mat->shared.occlusion_strength= m->occlusion_texture.scale > 0.0f ? m->occlusion_texture.scale : 1.0f;
    if(m->occlusion_texture.texcoord)
        mat->shared.has_occlusion_texcoord = true;
    memcpy(mat->shared.occlusion_scale, m->occlusion_texture.transform.scale, sizeof(vec2));

    // load iridescence
    if ((mat->has_iridescence = m->has_iridescence))
    {
        mat->iridescence.texture = load_texture_view(&m->iridescence.iridescence_texture, base_path, cache);
        mat->iridescence.thickness_texture = load_texture_view(&m->iridescence.iridescence_thickness_texture, base_path, cache);
        mat->iridescence.factor = m->iridescence.iridescence_factor;
        mat->iridescence.ior = m->iridescence.iridescence_ior;
        mat->iridescence.thickness_min = m->iridescence.iridescence_thickness_min;
        mat->iridescence.thickness_max = m->iridescence.iridescence_thickness_max;
    }

    // load sheen
    if ((mat->has_sheen = m->has_sheen))
    {
        mat->sheen.color_texture = load_texture_view(&m->sheen.sheen_color_texture, base_path, cache);
        mat->sheen.roughness_texture = load_texture_view(&m->sheen.sheen_roughness_texture, base_path, cache);
        memcpy(mat->sheen.color_factor, m->sheen.sheen_color_factor, sizeof(vec3));
        mat->sheen.roughness_factor = m->sheen.sheen_roughness_factor;
    }

    // load clearcoat
    if ((mat->has_clearcoat = m->has_clearcoat))
    {
        mat->clearcoat.texture = load_texture_view(&m->clearcoat.clearcoat_texture, base_path, cache);
        mat->clearcoat.roughness_texture = load_texture_view(&m->clearcoat.clearcoat_roughness_texture, base_path, cache);
        mat->clearcoat.normal_texture = load_texture_view(&m->clearcoat.clearcoat_normal_texture, base_path, cache);
        mat->clearcoat.factor = m->clearcoat.clearcoat_factor;
        mat->clearcoat.roughness_factor = m->clearcoat.clearcoat_roughness_factor;
    }

    // load volume
    if (m->has_volume)
    {
        mat->has_volume = m->has_volume;
        mat->volume.thickness_texture = load_texture_view(&m->volume.thickness_texture, base_path, cache);
        mat->volume.thickness_factor = m->volume.thickness_factor;
        mat->volume.attenuation_distance = m->volume.attenuation_distance;
        memcpy(mat->volume.attenuation_color, m->volume.attenuation_color, sizeof(vec3));
    }

    mat->double_sided = m->double_sided;
    mat->unlit = m->unlit;
}

#include "log.h"
#include "primitive.h"
#include <model.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "texture.h"

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
    // parse model file 
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
        log_error("ERROR... model_load() SAYS: NO MESHES IN FILE.");
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

    // count the amount of primitives on the model
    uint32_t total_primitives = 0;
    for (size_t mi = 0; mi < model_data->meshes_count; mi++)
    {
        cgltf_mesh *mesh = &model_data->meshes[mi];
        for (size_t pi = 0; pi < mesh->primitives_count; pi++)
        {
            if (mesh->primitives[pi].type == cgltf_primitive_type_triangles)
                total_primitives++;
        }
    }

    // break if model has no primitives
    if (total_primitives == 0)
    {
        log_error("model_load: no triangle primitives found in %s", location);
        cgltf_free(model_data);
        return 0;
    }

    // allocate space for all primitives
    model->primitives = (Primitive*)calloc(total_primitives, sizeof(Primitive));
    if (!model->primitives)
    {
        log_error("model_load: out of memory allocating %u primitives", total_primitives);
        cgltf_free(model_data);
        return 0;
    }

    TextureCache texture_cache;
    texture_cache_init(&texture_cache, 64);
    // load each primitive from the data into the model
    for (size_t mi = 0; mi < model_data->meshes_count; mi++)
    {
        cgltf_mesh* mesh = &model_data->meshes[mi];
        for (size_t pi = 0; pi < mesh->primitives_count; pi++)
        {
            cgltf_primitive* src = &mesh->primitives[pi];
 
            if (src->type != cgltf_primitive_type_triangles)
                continue;
 
            Primitive* dest = &model->primitives[model->primitive_count];
 
            if (!primitive_load(dest, src, base_path, &texture_cache))
            {
                log_error("model_load: primitive_load failed (mesh=%zu prim=%zu), skipping", mi, pi);
                primitive_free(dest);
                continue;
            }
 
            model->primitive_count++;
        }
    }

    cgltf_free(model_data);
 
    if (model->primitive_count == 0)
    {
        log_error("ERROR... model_load() SAYS: NO VALID PRIMITIVES TO LOAD FROM FILE %s", location);
        free(model->primitives);
        model->primitives = NULL;
        return 0;
    }
 
    // shrink model primitive alloc if some were skipped for any motives
    if (model->primitive_count < total_primitives)
    {
        Primitive* trimmed = (Primitive*)realloc(model->primitives, model->primitive_count * sizeof(Primitive));
        if (trimmed)
            model->primitives = trimmed;
    }

    log_info("model_load: loaded %u primitive(s) from %s", model->primitive_count, location);

    model->is_loaded = 1;

    texture_cache_free(&texture_cache);
    return 1;
}

void model_draw(const Model* model, Shader* shader)
{
    if (!model || !model->is_loaded || !model->primitives)
        return;
    // activate the shader
    shader_use(shader);

    // set 
    for (uint32_t primitive_index = 0; primitive_index < model->primitive_count; primitive_index++)
    {
        const Primitive* current_primitive = &model->primitives[primitive_index];
        
        bool has_albedo = false;
        bool has_metallic_roughness = false;
        bool has_normal = false;
        bool has_emissive = false;
        bool has_ao = false;

        if (current_primitive->albedo)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, current_primitive->albedo);
            shader_set_int(shader, "u_albedo", 0);
            shader_set_vec4(shader, "u_albedo_factor", current_primitive->albedo_factor);
            has_albedo = true;
        }
        if(current_primitive->metallic_roughness)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, current_primitive->metallic_roughness);
            shader_set_int(shader, "u_metallic_roughness", 1);
            shader_set_float(shader, "u_metallic_factor", current_primitive->metallic_factor);
            shader_set_float(shader, "u_roughness_factor", current_primitive->roughness_factor);
            has_metallic_roughness = true;
        }
        if(current_primitive->normal)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, current_primitive->normal);
            shader_set_int(shader, "u_normal", 2);
            has_normal = true;
        }
        if(current_primitive->emissive)
        {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, current_primitive->emissive);
            shader_set_int(shader, "u_emissive", 3);
            shader_set_float(shader, "u_emissive_strength", current_primitive->emissive_strength);
            shader_set_vec3(shader, "u_emissive_factor", current_primitive->emissive_factor);
            has_emissive = true;
        }
        if(current_primitive->ambient_occlusion)
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, current_primitive->ambient_occlusion);
            shader_set_int(shader, "u_ao", 4);
            has_ao = true;
        }
        
        shader_set_bool(shader, "u_has_albedo", has_albedo);
        shader_set_bool(shader, "u_has_metallic_roughness", has_metallic_roughness);
        shader_set_bool(shader, "u_has_normal", has_normal);
        shader_set_bool(shader, "u_has_emissive", has_emissive);
        shader_set_bool(shader, "u_has_ao", has_ao);

        glActiveTexture(GL_TEXTURE0);

        glBindVertexArray(current_primitive->VAO);
        glDrawElements(GL_TRIANGLES, current_primitive->index_count, current_primitive->index_type, (void *)0);
        glBindVertexArray(0);
    }

}

void model_free(Model *model)
{
    if (!model) return;
 
    // delete each primitive from the model
    for (uint32_t i = 0; i < model->primitive_count; i++)
        primitive_free(&model->primitives[i]);
 
    // deallocate primitive memory
    free(model->primitives);

    // reset struct
    memset(model, 0, sizeof(*model));
}
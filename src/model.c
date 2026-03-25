#include "log.h"
#include "primitive.h"
#include <model.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "texture_loader.h"

int model_load(Model* model, const char* location)
{    
    if(!model || !location)
        return 0;

    // set all memory to 0
    memset(model, 0, sizeof(*model));
    strncpy(model->filepath, location, MAX_MODEL_PATH_LENGTH - 1);
    model->filepath[MAX_MODEL_PATH_LENGTH - 1] = '\0';

    clock_t time = clock();
    model->is_loaded = 0;

    cgltf_options model_options = {0};
    cgltf_data* model_data = NULL;

    // parse model file 
    cgltf_result result = cgltf_parse_file(&model_options, location, &model_data);
    if(result != cgltf_result_success)
    {
        log_error("ERROR... model_load() SAYS: cgltf_parse_file FAILED. RESULT = %d", result);
        cgltf_free(model_data);
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

    uint32_t total_meshes = 0;
    model->model_primitive_count = 0;
    // count the amount of primitives on the model
    for (size_t ni = 0; ni < model_data->nodes_count; ni++)
    {
        cgltf_node* node = &model_data->nodes[ni];
        if(!node->mesh)
            continue;
        // get the total amount of primitives per model
        for (size_t pi = 0; pi < node->mesh->primitives_count; pi++)
        {
            if (node->mesh->primitives[pi].type == cgltf_primitive_type_triangles)
                model->model_primitive_count++;
        }
        total_meshes++;
    }

    // allocate space for all meshes
    model->meshes = (Mesh*)calloc(total_meshes, sizeof(Mesh));
    if (!model->meshes)
    {
        log_error("model_load: out of memory allocating %u primitives", total_meshes);
        cgltf_free(model_data);
        return 0;
    }

    TextureCache texture_cache;
    texture_cache_init(&texture_cache, 64);
    texture_decode_all(model_data, base_path, &texture_cache);
    // iterate each node from a model
    for(uint64_t ni = 0; ni < model_data->nodes_count; ni++)
    {
        static clock_t node_time = 0;
        node_time = clock();
        cgltf_node* src_node = &model_data->nodes[ni];
        if(!src_node->mesh)
            continue;

        //mat4 world_transform;
        //node_world_transform(src_node, world_transform);

        cgltf_mesh* src_mesh = src_node->mesh;
        Mesh* dest_mesh = &model->meshes[model->mesh_count];

        cgltf_node_transform_world(src_node, (float*)dest_mesh->transform);

        // count each primitive on a mesh
        uint32_t primitive_count = 0;
        for(size_t pi = 0; pi < src_mesh->primitives_count; pi++)
            if(src_mesh->primitives[pi].type == cgltf_primitive_type_triangles)
                primitive_count++;
        if(primitive_count == 0)
            continue;
        // allocate memory for primitives on a per-mesh basis
        dest_mesh->primitives = (Primitive*)calloc(primitive_count, sizeof(Primitive));
        if(!dest_mesh->primitives)
        {
            log_error("ERROR... model_load() SAYS: OUT OF MEMORY FOR PRIMITIVES AT NODE %zu", ni);
            continue;
        }

        // iterate each mesh
        for (uint64_t pi = 0; pi < src_mesh->primitives_count; pi++)
        {
            cgltf_primitive* src_primitive = &src_mesh->primitives[pi];
            if (src_primitive->type != cgltf_primitive_type_triangles)
                continue;
        
            Primitive* dest_primitive = &dest_mesh->primitives[dest_mesh->primitive_count];
 
            if (!primitive_load(dest_primitive, src_primitive, base_path, &texture_cache))
            {
                log_error("ERROR... model_load() SAYS: primitive_load FAILED AT NODE %zu PRIM %zu", ni, pi);
                primitive_free(dest_primitive);
                continue;
            }
            dest_mesh->primitive_count++;
        }

        if(dest_mesh->primitive_count == 0)
        {
            free(dest_mesh->primitives);
            dest_mesh->primitives = NULL;
            continue;
        }

        model->mesh_count++;
        node_time = clock() - node_time;
        log_info("model_load() SAYS: LOADED %u PRIMITIVE(S) FROM MESH NUMBER %u IN %.4f", dest_mesh->primitive_count, model->mesh_count, (double)node_time/CLOCKS_PER_SEC);
    }

    cgltf_free(model_data);
 
    if (model->mesh_count == 0)
    {
        log_error("ERROR... model_load() SAYS: NO VALID PRIMITIVES TO LOAD FROM FILE %s", location);
        free(model->meshes);
        model->meshes = NULL;
        return 0;
    }

    time = clock() - time;
    log_info("model_load() SAYS: LOADED %u MESHES IN %.3f SECONDS. SOURCE: %s", model->mesh_count, (float)time / CLOCKS_PER_SEC, location);

    model->is_loaded = 1;

    texture_cache_free(&texture_cache);
    return 1;
}

void model_draw(const Model* model, Shader* shader, mat4 world_matrix)
{
    if (!model || !model->is_loaded || !model->meshes)
        return;
    // activate the shader
    shader_use(shader);

    // draw each primitive from a model
    for (uint32_t mi = 0; mi < model->mesh_count; mi++)
    {
        const Mesh* mesh = &model->meshes[mi];

        // multiply each model's mesh matrix by the world matrix of the entity that owns them
        mat4 final_transform;
        // avx makes EVERYTHING explode and i am too lazy to be bothered :D
        glm_mat4_mul_sse2(world_matrix, (vec4*)mesh->transform, final_transform);

        shader_set_mat4(shader, "u_model", final_transform);
        
        for(uint32_t pi = 0; pi < mesh->primitive_count; pi++)
        {
            const Primitive* current_primitive = &mesh->primitives[pi];

            bool has_albedo = false;
            bool has_metallic_roughness = false;
            bool has_normal = false;
            bool has_emissive = false;
            bool has_ao = false;

            if (current_primitive->material.pbr.albedo)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.pbr.albedo);
                shader_set_int(shader, "u_albedo", 0);
                has_albedo = true;
            }
            if(current_primitive->material.pbr.metallic_roughness)
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.pbr.metallic_roughness);
                shader_set_int(shader, "u_metallic_roughness", 1);
                has_metallic_roughness = true;
            }
            if(current_primitive->material.shared.normal)
            {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.shared.normal);
                shader_set_int(shader, "u_normal", 2);
                shader_set_vec2(shader, "u_normal_scale", current_primitive->material.shared.normal_scale);
                has_normal = true;
            }
            if(current_primitive->material.shared.emissive)
            {
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.shared.emissive);
                shader_set_int(shader, "u_emissive", 3);
                shader_set_bool(shader, "u_has_emissive_texcoord", current_primitive->material.shared.has_emissive_texcoord);
                has_emissive = true;
            }
            if(current_primitive->material.shared.ambient_occlusion)
            {
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.shared.ambient_occlusion);
                shader_set_int(shader, "u_ao", 4);
                shader_set_float(shader, "u_occlusion_strength", current_primitive->material.shared.occlusion_strength);
                shader_set_vec2(shader, "u_occlusion_scale", current_primitive->material.shared.occlusion_scale);
                shader_set_bool(shader, "u_has_occlusion_texcoord", current_primitive->material.shared.has_occlusion_texcoord);
                has_ao = true;
            }
            if(current_primitive->material.has_iridescence)
            {
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.iridescence.texture);
                shader_set_int(shader, "u_iridescence", 5);
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, current_primitive->material.iridescence.thickness_texture);
                shader_set_int(shader, "u_iridescence_thickness", 6);
                
                shader_set_float(shader, "u_iridescence_thickness_max", current_primitive->material.iridescence.thickness_max);
                shader_set_float(shader, "u_iridescence_thickness_min", current_primitive->material.iridescence.thickness_min);
                shader_set_float(shader, "u_iridescence_factor", current_primitive->material.iridescence.factor);
                shader_set_float(shader, "u_iridescence_ior", current_primitive->material.iridescence.ior);
            }

            shader_set_bool(shader, "u_has_albedo", has_albedo);
            shader_set_vec4(shader, "u_albedo_factor", current_primitive->material.pbr.albedo_factor);

            shader_set_bool(shader, "u_has_metallic_roughness", has_metallic_roughness);
            shader_set_float(shader, "u_metallic_factor", current_primitive->material.pbr.metallic_factor);
            shader_set_float(shader, "u_roughness_factor", current_primitive->material.pbr.roughness_factor);

            shader_set_bool(shader, "u_has_normal", has_normal);

            shader_set_bool(shader, "u_has_emissive", has_emissive);
            shader_set_float(shader, "u_emissive_strength", current_primitive->material.shared.emissive_strength);
            shader_set_vec3(shader, "u_emissive_factor", current_primitive->material.shared.emissive_factor);

            shader_set_bool(shader, "u_has_ao", has_ao);

            shader_set_bool(shader, "u_has_iridescence", current_primitive->material.has_iridescence);

            shader_set_bool(shader, "u_unlit", current_primitive->material.unlit);

            glActiveTexture(GL_TEXTURE0);

            glBindVertexArray(current_primitive->VAO);
            glDrawElements(GL_TRIANGLES, current_primitive->index_count, current_primitive->index_type, (void*)0);
            glBindVertexArray(0);
        }
    }

}

void model_free(Model *model)
{
    if (!model) return;
 
    // delete each mesh from the model
    for (uint32_t i = 0; i < model->mesh_count; i++)
        mesh_free(&model->meshes[i]);
 
    // deallocate mesh memory
    free(model->meshes);

    // reset struct
    memset(model, 0, sizeof(*model));
}
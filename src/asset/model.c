#include "log.h"
#include "primitive.h"
#include <model.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <cglm/cglm.h>

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
        cgltf_node* src_node = &model_data->nodes[ni];
        if(!src_node->mesh)
            continue;

        //mat4 world_transform;
        //node_world_transform(src_node, world_transform);

        cgltf_mesh* src_mesh = src_node->mesh;
        Mesh* dest_mesh = &model->meshes[model->mesh_count];
        glm_vec3_fill(dest_mesh->aabb[0], FLT_MAX);
        glm_vec3_fill(dest_mesh->aabb[1], -FLT_MAX);

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

            // grab the mesh aabb
            glm_vec3_minv(dest_mesh->aabb[0], src_primitive->attributes->data->min, dest_mesh->aabb[0]);
            glm_vec3_maxv(dest_mesh->aabb[1], src_primitive->attributes->data->max, dest_mesh->aabb[1]);

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

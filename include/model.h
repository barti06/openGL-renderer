#ifndef MODEL_H
#define MODEL_H

#include "primitive.h"
#include "shader.h"

typedef struct Model
{
    // store the gltf model meshes
    Mesh* meshes;
    uint32_t mesh_count;
    uint32_t model_primitive_count;

    // store path of the loaded model
    char filepath[MAX_MODEL_PATH_LENGTH];
    // simple loading flag
    int is_loaded;
} Model;

// stores a model in the first parameter, given 
// in a location set by the second parameter
int model_load(Model* model, const char* location);

// this function takes a model for drawing and a shader to draw to
void model_draw(const Model* model, Shader* shader, mat4 world_matrix);

// this function deletes everything related to the model
void model_free(Model* model);
#endif
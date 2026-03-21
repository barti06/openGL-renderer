#ifndef MODEL_H
#define MODEL_H

#include "cgltf.h"
#include "stb_image.h"

#include <glad/glad.h>
#include <shader.h>

#define MAX_MODEL_PATH_LENGTH 512

typedef struct Model
{
    // store path of the loaded model
    char filepath[MAX_MODEL_PATH_LENGTH];
    // simple loading flag
    int is_loaded;

    // openGL handles
    GLuint VAO;
    GLuint VBO_positions;
    GLuint VBO_uvs;
    GLuint VBO_normals;
    GLuint EBO;
    GLuint albedo;

    // num of instances to draw
    GLsizei index_count;
    GLenum  index_type;
} Model;

// stores a model in the first parameter, given 
// in a location set by the second parameter
int model_load(Model* model, const char* location);

// this function takes a model for drawing and a shader to draw to
void model_draw(const Model* model, Shader* shader);

// this function deletes everything related to the model
void model_free(Model* model);
#endif
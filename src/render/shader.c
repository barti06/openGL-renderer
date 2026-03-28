#include "shader.h"
#include "utils.h"

void shader_init(Shader* shader, const char* vertexPath, const char* fragmentPath)
{
    // try loading the shader code into this function
    if(vertexPath == NULL || fragmentPath == NULL)
        log_error("\nShader init_shader ERROR: INVALID SHADER PATHS!");

    const char* fragmentCode = read_file(fragmentPath);
    const char* vertexCode = read_file(vertexPath);

    if(vertexCode == NULL || fragmentCode == NULL)
        log_error("\nShader init_shader ERROR: COULDN'T READ SHADER CODE!");

    // set the shader to send
    shader->frag_path = fragmentPath;
    shader->vert_path = vertexPath;

    // the actual openGL shaders
    uint32_t vertex, fragment;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexCode, NULL);
    glCompileShader(vertex);
    // delete vertex shader code from memory, as it's now fully loaded
    free((void*)vertexCode);
    // checks if vertex shader was compiled successfully
    int32_t successVert;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &successVert);
    if (!successVert)
    {
        // gets the error in question and writes it to a log
        char infoLogVert[512];
        glGetShaderInfoLog(vertex, 512, NULL, infoLogVert);
        FILE* fptr = fopen("shaderLog.txt", "a");
        if (fptr != NULL)
        {
            fprintf(fptr, "\n%s", infoLogVert);
            fclose(fptr);
        }
        glDeleteShader(vertex);
        log_error("\nShader shader_init ERROR: VERTEX SHADER COMPILATION FAILED. SEE LOG FOR MORE INFO.");
        return;
    }

    // creates a fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentCode, NULL);
    glCompileShader(fragment);
    // delete fragment shader code from memory, as it's now fully loaded
    free((void*)fragmentCode);
    // checks if fragment shader was compiled sucessfully
    int32_t successFrag;
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &successFrag);
    if (!successFrag)
    {
        char infoLogFrag[512];
        glGetShaderInfoLog(fragment, 512, NULL, infoLogFrag);
        FILE* fptr = fopen("shaderLog.txt", "a");
        if (fptr != NULL)
        {
            fprintf(fptr, "\n%s", infoLogFrag);
            fclose(fptr);
        }
        glDeleteShader(fragment);
        log_error("Shader shader_init ERROR: FRAGMENT SHADER COMPILATION FAILED. SEE LOG FOR MORE INFO.");
        return;
    }
    // create a shader program and attach the vertex and fragment shaders
    shader->ID = glCreateProgram();
    glAttachShader(shader->ID, vertex);
    glAttachShader(shader->ID, fragment);

    // link the program
    glLinkProgram(shader->ID);
    // check if shader program linked successfully
    int32_t successProgram;
    glGetProgramiv(shader->ID, GL_LINK_STATUS, &successProgram);
    if (!successProgram)
    {
        char infoLogProgram[512];
        glGetProgramInfoLog(shader->ID, 512, NULL, infoLogProgram);
        FILE* fptr = fopen("shaderLog.txt", "a");
        if (fptr != NULL)
        {
            fprintf(fptr, "\n%s", infoLogProgram);
            fclose(fptr);
        }
        log_error("Shader shader_init ERROR: SHADER PROGRAM LINKING FAILED. SEE LOG FOR MORE INFO.");
    }
    
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return;
}

void shader_reload_frag(Shader* shader)
{
    // try loading the shader code into this function
    if(shader->frag_path == NULL)
        log_error("\nShader shader_reload_frag() ERROR: INVALID SHADER PATHS!");

    const char* fragmentCode = read_file(shader->frag_path);
    if(fragmentCode == NULL)
        log_error("\nShader shader_reload_frag() ERROR: COULDN'T READ SHADER CODE!");

    // the actual openGL shaders
    uint32_t fragment;
    // creates a fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentCode, NULL);
    glCompileShader(fragment);
    // delete shader code from memory, as it's now fully loaded
    free((void*)fragmentCode);
    // checks if fragment shader was compiled sucessfully
    int32_t successFrag;
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &successFrag);
    if (!successFrag)
    {
        char infoLogFrag[512];
        glGetShaderInfoLog(fragment, 512, NULL, infoLogFrag);
        FILE* fptr = fopen("shaderLog.txt", "a");
        if (fptr != NULL)
        {
            fprintf(fptr, "\n%s", infoLogFrag);
            fclose(fptr);
        }
        // delete broken shader
        glDeleteShader(fragment);
        log_error("Shader shader_init ERROR: FRAGMENT SHADER COMPILATION FAILED. SEE LOG FOR MORE INFO.");
        return;
    }

    // deattach and delete the old fragment shader
    GLint attachedCount;
    GLuint attached[4] = { 0 };
    glGetAttachedShaders(shader->ID, 4, &attachedCount, attached);
    for (int i = 0; i < attachedCount; i++) 
    {
        GLint type;
        glGetShaderiv(attached[i], GL_SHADER_TYPE, &type);
        if(type == GL_FRAGMENT_SHADER)
        {
            glDetachShader(shader->ID, attached[i]);
            glDeleteShader(attached[i]);
        }
    }

    // create a shader program and attach the vertex and fragment shaders
    glAttachShader(shader->ID, fragment);

    glLinkProgram(shader->ID);
    // check if shader program linked successfully
    int32_t successProgram;
    glGetProgramiv(shader->ID, GL_LINK_STATUS, &successProgram);
    if (!successProgram)
    {
        char infoLogProgram[512];
        glGetProgramInfoLog(shader->ID, 512, NULL, infoLogProgram);
        FILE* fptr = fopen("shaderLog.txt", "a");
        if (fptr != NULL)
        {
            fprintf(fptr, "\n%s", infoLogProgram);
            fclose(fptr);
        }
        log_error("Shader shader_init ERROR: SHADER PROGRAM LINKING FAILED. SEE LOG FOR MORE INFO.");
    }
    
    // delete the shader as it's linked into the program now and no longer necessary
    glDeleteShader(fragment);
}

void shader_use(Shader* shader)
{
    glUseProgram(shader->ID);
}

void shader_set_bool(Shader* shader, const char* name, bool value)
{
    glUniform1i(glGetUniformLocation(shader->ID, name), (int)value);
}

void shader_set_int(Shader* shader, const char* name, int value)
{
    glUniform1i(glGetUniformLocation(shader->ID, name), value);
}

void shader_set_float(Shader* shader, const char* name, float value)
{
    glUniform1f(glGetUniformLocation(shader->ID, name), value);
}

void shader_set_vec2(Shader* shader, const char* name, const vec2 value)
{
    glUniform2fv(glGetUniformLocation(shader->ID, name), 1, value);
}

void shader_set_vec3(Shader* shader, const char* name, const vec3 value)
{
    glUniform3fv(glGetUniformLocation(shader->ID, name), 1, value);
}

void shader_set_vec4(Shader* shader, const char* name, const vec4 value)
{
    glUniform4fv(glGetUniformLocation(shader->ID, name), 1, value);
}

void shader_set_mat3(Shader* shader, const char* name, const mat3 value)
{
    glUniformMatrix3fv(glGetUniformLocation(shader->ID, name), 1, GL_FALSE, (const float*)value);
}

void shader_set_mat4(Shader* shader, const char* name, const mat4 value)
{
    // cglm passes matrices as arrays of vectors, which then decay to pointers, making this safe
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, name), 1, GL_FALSE, (const float*)value);
}

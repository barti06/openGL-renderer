#include <shader.h>

Shader init_shader(const char* vertexPath, const char* fragmentPath)
{
    // try loading the shader code into this function
    if(vertexPath == NULL || fragmentPath == NULL)
        log_error("\nShader init_shader ERROR: INVALID SHADER PATHS!");

    const char* fragmentCode = read_file(fragmentPath);
    const char* vertexCode = read_file(vertexPath);

    if(vertexCode == NULL || fragmentCode == NULL)
        log_error("\nShader init_shader ERROR: COULDN'T READ SHADER CODE!");

    // set the shader to send
    Shader shader;
    shader.frag_path = fragmentPath;
    shader.vert_path = vertexPath;

    // the actual openGL shaders
    uint32_t vertex, fragment;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexCode, NULL);
    glCompileShader(vertex);
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
        log_error("\nShader shader_init ERROR: VERTEX SHADER COMPILATION FAILED. SEE LOG FOR MORE INFO.");
    }

    // creates a fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentCode, NULL);
    glCompileShader(fragment);
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
        log_error("Shader shader_init ERROR: FRAGMENT SHADER COMPILATION FAILED. SEE LOG FOR MORE INFO.");
    }
    // create a shader program and attach the vertex and fragment shaders
    shader.ID = glCreateProgram();
    glAttachShader(shader.ID, vertex);
    glAttachShader(shader.ID, fragment);

    // link the program
    glLinkProgram(shader.ID);
    // check if shader program linked successfully
    int32_t successProgram;
    glGetProgramiv(shader.ID, GL_COMPILE_STATUS, &successProgram);
    if (!successProgram)
    {
        char infoLogProgram[512];
        glGetShaderInfoLog(shader.ID, 512, NULL, infoLogProgram);
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

    return shader;
}

void use_shader(Shader* shader)
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

void shader_set_vec2(Shader* shader, const char* name, vec2 value)
{
    glUniform2fv(glGetUniformLocation(shader->ID, name), 1, value);
}

void shader_set_vec3(Shader* shader, const char* name, vec3 value)
{
    glUniform3fv(glGetUniformLocation(shader->ID, name), 1, value);
}

void shader_set_vec4(Shader* shader, const char* name, vec4 value)
{
    glUniform4fv(glGetUniformLocation(shader->ID, name), 1, value);
}

void shader_set_mat3(Shader* shader, const char* name, mat3 value)
{
    glUniformMatrix3fv(glGetUniformLocation(shader->ID, name), 1, GL_FALSE, (const float*)value);
}

void shader_set_mat4(Shader* shader, const char* name, mat4 value)
{
    // cglm passes matrices as arrays of vectors, which then decay to pointers, making this safe
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, name), 1, GL_FALSE, (const float*)value);
}
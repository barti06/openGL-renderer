#include <rtGL/shader.h>

void Shader::init(const char* vertexPath, const char* fragmentPath)
{
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    this->m_fragPath = fragmentPath;
    this->m_vertPath = vertexPath;
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    // start the process for compiling shaders
    uint32_t vertex, fragment;

    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    // checks if vertex shader was compiled successfully
    int32_t successVert;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &successVert);
    if (!successVert)
    {
        // gets the error in question and writes it to a log
        char infoLogVert[512]{};
        glGetShaderInfoLog(vertex, 512, NULL, infoLogVert);
        FILE* fptr{};
        errno_t err{fopen_s(&fptr, "shaderLog.txt", "a")};
        if (err == 0)
        {
            fprintf(fptr, "\n%s", infoLogVert);
            fclose(fptr);
        }
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        /*
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        */
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        throw std::runtime_error("VERTEX SHADER COMPILATION FAILED. SEE LOG FOR MORE INFO.");
    }

    // creates a fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    // checks if fragment shader was compiled sucessfully
    int32_t successFrag;
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &successFrag);
    if (!successFrag)
    {
        char infoLogFrag[512]{};
        glGetShaderInfoLog(fragment, 512, NULL, infoLogFrag);
        FILE* fptr{};
        errno_t err{ fopen_s(&fptr, "shaderLog.txt", "a") };
        if (err == 0)
        {
            fprintf(fptr, "\n%s", infoLogFrag);
            fclose(fptr);
        }
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        /*
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        */
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        throw std::runtime_error("FRAGMENT SHADER COMPILATION FAILED. SEE LOG FOR MORE INFO.");
    }
    // create a shader program and attach the vertex and fragment shaders
    this->m_ID = glCreateProgram();
    glAttachShader(this->m_ID, vertex);
    glAttachShader(this->m_ID, fragment);

    // link the program
    glLinkProgram(this->m_ID);
    // check if shader program linked successfully
    int32_t successProgram;
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &successProgram);
    if (!successProgram)
    {
        char infoLogProgram[512]{};
        glGetShaderInfoLog(this->m_ID, 512, NULL, infoLogProgram);
        FILE* fptr{};
        errno_t err{ fopen_s(&fptr, "shaderLog.txt", "a") };
        if (err == 0)
        {
            fprintf(fptr, "\n%s", infoLogProgram);
            fclose(fptr);
        }
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        /*
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        NOTE: REMEMBER TO MOVE THIS TO A NEW "SHADER INITIALIZATION" FUNCTION!!!!!!!!!
        */
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        // shouldn't have used throws in constructor :/ 
        throw std::runtime_error("SHADER PROGRAM LINKING FAILED. SEE LOG FOR MORE INFO.");
    }
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::add_geom(const char* geomPath)
{
    // retrieve the vertex/fragment source code from filePath
    std::string geometryCode;
    std::ifstream gShaderFile;
    // ensure ifstream objects can throw exceptions:
    gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        gShaderFile.open(geomPath);
        std::stringstream gShaderStream;
        // read file's buffer contents into streams
        gShaderStream << gShaderFile.rdbuf();
        // close file handlers
        gShaderFile.close();
        // convert stream into string
        geometryCode = gShaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    this->m_geomPath = geomPath;
    const char* gShaderCode = geometryCode.c_str();
    // start the process for compiling shaders
    uint32_t geometry;

    // creates a fragment Shader
    geometry = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometry, 1, &gShaderCode, NULL);
    glCompileShader(geometry);
    // checks if fragment shader was compiled sucessfully
    int32_t successGeom;
    glGetShaderiv(geometry, GL_COMPILE_STATUS, &successGeom);
    if (!successGeom)
    {
        char infoLogGeom[512]{};
        glGetShaderInfoLog(geometry, 512, NULL, infoLogGeom);
        FILE* fptr{};
        errno_t err{ fopen_s(&fptr, "shaderLog.txt", "a") };
        if (err == 0)
        {
            fprintf(fptr, "\n%s", infoLogGeom);
            fclose(fptr);
        }
        throw std::runtime_error("GEOMETRY SHADER COMPILATION FAILED. SEE LOG FOR MORE INFO.");
    }
    // attach the new geometry shader to the program
    glAttachShader(this->m_ID, geometry);

    // link the program
    glLinkProgram(this->m_ID);
    // check if shader program linked successfully
    int32_t successProgram;
    glGetProgramiv(this->m_ID, GL_LINK_STATUS, &successProgram);
    if (!successProgram)
    {
        char infoLogProgram[512]{};
        glGetProgramInfoLog(this->m_ID, 512, NULL, infoLogProgram);
        FILE* fptr{};
        errno_t err{ fopen_s(&fptr, "shaderLog.txt", "a") };
        if (err == 0)
        {
            fprintf(fptr, "\n%s", infoLogProgram);
            fclose(fptr);
        }
        throw std::runtime_error("SHADER PROGRAM LINKING FAILED. SEE LOG FOR MORE INFO.");
    }
    // delete the shader as it has already linked into our program no longer serves a purpose
    glDeleteShader(geometry);
}


void Shader::recompileFragment()
{
    // retrieve the vertex/fragment source code from filePath
    std::string fragmentCode;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        fShaderFile.open(this->m_fragPath);
        std::stringstream fShaderStream;
        // read file's buffer contents into streams
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        fShaderFile.close();
        // convert stream into string
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    const char* fShaderCode = fragmentCode.c_str();
    // start the process for compiling shaders
    uint32_t fragment;

    // creates a fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    // checks if fragment shader was compiled sucessfully
    int32_t successFrag;
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &successFrag);
    if (!successFrag)
    {
        char infoLogFrag[512]{};
        glGetShaderInfoLog(fragment, 512, NULL, infoLogFrag);
        FILE* fptr{};
        errno_t err{ fopen_s(&fptr, "shaderLog.txt", "a") };
        if (err == 0)
        {
            fprintf(fptr, "\n%s", infoLogFrag);
            fclose(fptr);
        }
        throw std::runtime_error("FRAGMENT SHADER COMPILATION FAILED. SEE LOG FOR MORE INFO.");
    }

    // detach the old fragment shader before attaching the new one
    GLint attachedCount;
    GLuint attached[8] = { 0 };
    glGetAttachedShaders(this->m_ID, 8, &attachedCount, attached);
    for (int i = 0; i < attachedCount; i++) {
        GLint type;
        glGetShaderiv(attached[i], GL_SHADER_TYPE, &type);
        if (type == GL_FRAGMENT_SHADER)
            glDetachShader(this->m_ID, attached[i]);
    }

    glAttachShader(this->m_ID, fragment);

    // link the program
    glLinkProgram(this->m_ID);
    // check if shader program linked successfully
    int32_t successProgram;
    glGetProgramiv(this->m_ID, GL_LINK_STATUS, &successProgram);
    if (!successProgram)
    {
        char infoLogProgram[512]{};
        glGetProgramInfoLog(this->m_ID, 512, NULL, infoLogProgram);
        FILE* fptr{};
        errno_t err{ fopen_s(&fptr, "shaderLog.txt", "a") };
        if (err == 0)
        {
            fprintf(fptr, "\n%s", infoLogProgram);
            fclose(fptr);
        }
        throw std::runtime_error("SHADER PROGRAM LINKING FAILED. SEE LOG FOR MORE INFO.");
    }
    // delete the shader as it has already linked into our program no longer serves a purpose
    glDeleteShader(fragment);
}

uint32_t& Shader::getID()
{
    return this->m_ID;
}

void Shader::use()
{
    glUseProgram(this->m_ID);
}

/* set utility */ //NOTE: may add more
/*----------------------------------------------------------*/
void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(this->m_ID, name.c_str()), (int)value);
}
/*----------------------------------------------------------*/
void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(this->m_ID, name.c_str()), value);
}
/*----------------------------------------------------------*/
void Shader::setFloat1(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(this->m_ID, name.c_str()), value);
}
/*----------------------------------------------------------*/
void Shader::setFloat2(const std::string& name, float f1, float f2) const
{
    glUniform2f(glGetUniformLocation(this->m_ID, name.c_str()), f1, f2);
}
/*----------------------------------------------------------*/
void Shader::setFloat3(const std::string& name, float f1, float f2, float f3) const
{
    glUniform3f(glGetUniformLocation(this->m_ID, name.c_str()), f1, f2, f3);
}
/*----------------------------------------------------------*/
void Shader::setFloat4(const std::string& name, float f1, float f2, float f3, float f4) const
{
    glUniform4f(glGetUniformLocation(this->m_ID, name.c_str()), f1, f2, f3, f4);
}
/*----------------------------------------------------------*/
void Shader::setVec2(const std::string& name, glm::vec2& vec) const
{ 
    glUniform2f(glGetUniformLocation(this->m_ID, name.c_str()), vec.x, vec.y);
}
/*----------------------------------------------------------*/
void Shader::setVec3(const std::string& name, glm::vec3& vec) const
{
    glUniform3f(glGetUniformLocation(this->m_ID, name.c_str()), vec.x, vec.y, vec.z);
}
/*----------------------------------------------------------*/
void Shader::setVec4(const std::string& name, glm::vec4& vec) const
{
    glUniform4f(glGetUniformLocation(this->m_ID, name.c_str()), vec.x, vec.y, vec.z, vec.w);
}
/*----------------------------------------------------------*/
void Shader::setMat2(const std::string& name, glm::mat2& mat) const
{
    glUniformMatrix2fv(glGetUniformLocation(this->m_ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
/*----------------------------------------------------------*/
void Shader::setMat3(const std::string& name, glm::mat3& mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(this->m_ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
/*----------------------------------------------------------*/
void Shader::setMat4(const std::string& name, glm::mat4& mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(this->m_ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
/*----------------------------------------------------------*/
/* end set utility */


Shader::~Shader()
{
    glDeleteProgram(this->m_ID);
}
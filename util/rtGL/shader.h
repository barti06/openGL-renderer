#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h> // include glad to get all the required OpenGL headers
#include <util.h>


class Shader
{
public:
    // constructor reads and builds the shader
    void init(const char* vertexPath, const char* fragmentPath);

    void add_geom(const char* geomPath);

    void recompileFragment();
    // use/activate the shader
    void use();

    // utility uniform functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat1(const std::string& name, float value) const;
    void setFloat2(const std::string& name, float f1, float f2) const;
    void setFloat3(const std::string& name, float f1, float f2, float f3) const;
    void setFloat4(const std::string& name, float f1, float f2, float f3, float f4) const;
    void setVec2(const std::string& name, glm::vec2& vec) const;
    void setVec3(const std::string& name, glm::vec3& vec) const;
    void setVec4(const std::string& name, glm::vec4& vec) const;
    void setMat2(const std::string& name, glm::mat2& mat) const;
    void setMat3(const std::string& name, glm::mat3& mat) const;
    void setMat4(const std::string& name, glm::mat4& mat) const;

    // get the shader's ID by reference
    uint32_t& getID();
    
    // destructor
    ~Shader();

private:
    std::string m_vertPath{};
    std::string m_geomPath{};
    std::string m_fragPath{};
    // store the shader id
    uint32_t m_ID{};
};

#endif
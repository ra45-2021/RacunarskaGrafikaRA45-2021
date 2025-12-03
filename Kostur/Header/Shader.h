#pragma once
#include <string>
#include <GL/glew.h>

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);
    void use();
    void setFloat(const std::string &name, float value);
    void setVec3(const std::string& name, float x, float y, float z);
    void setMat4(const std::string &name, const float* value);
};

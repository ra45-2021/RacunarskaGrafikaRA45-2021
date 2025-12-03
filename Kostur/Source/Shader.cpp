#include "../Header/Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(const char* vPath, const char* fPath)
{
    std::ifstream vFile(vPath);
    std::ifstream fFile(fPath);

    std::stringstream vStream, fStream;
    vStream << vFile.rdbuf();
    fStream << fFile.rdbuf();

    std::string vertexCode = vStream.str();
    std::string fragmentCode = fStream.str();

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() {
    glUseProgram(ID);
}

void Shader::setFloat(const std::string &name, float value) {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setMat4(const std::string &name, const float* value)
{
    glUniformMatrix4fv(
        glGetUniformLocation(ID, name.c_str()),
        1,
        GL_FALSE,
        value
    );
}

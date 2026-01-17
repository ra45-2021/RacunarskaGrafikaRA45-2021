#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

struct MeshGL {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei vertexCount = 0; 
    bool isCubeFans = false; 
};

struct Renderer {
    MeshGL cube;
    MeshGL basin;
    MeshGL water;
    MeshGL dropletSphere;
    MeshGL overlayQuad;

    GLuint overlayTex = 0;

    GLuint shader = 0;
    GLint uM = -1, uV = -1, uP = -1;
    GLint uUseTex = -1, uTransparent = -1, uTint = -1;
    GLint uTex = -1;

    MeshGL centerQuad;
    GLuint centerTex = 0;
    MeshGL screenQuad;

    bool Init(GLuint shaderProgram, const char* overlayPath);
    void Destroy();

    void CreateCube();
    void CreateFromFloats(MeshGL& m, const std::vector<float>& data, bool cubeFans = false);

    void SetCommonUniforms(const glm::mat4& V, const glm::mat4& P);
    void DrawCube(const glm::mat4& M, const glm::vec4& tint, bool transparent);
    void DrawMeshTriangles(const MeshGL& m, const glm::mat4& M, const glm::vec4& tint, bool transparent);
    void DrawOverlay();
    void DrawTexturedMesh(const MeshGL& m, const glm::mat4& M, GLuint texID, const glm::vec4& tint = glm::vec4(1.0f));
    void DrawCenter();
    void CreateScreenQuad();
    void DrawTexturedScreen(const glm::mat4& M, GLuint texID, const glm::vec4& tint = glm::vec4(1,1,1,1));

};

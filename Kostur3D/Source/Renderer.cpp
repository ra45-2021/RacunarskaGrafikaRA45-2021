#include "Header/Renderer.h"
#include "Util.h"
#include <glm/gtc/type_ptr.hpp>

static constexpr GLsizei STRIDE_FLOATS = 12;
static constexpr GLsizei STRIDE_BYTES  = STRIDE_FLOATS * (GLsizei)sizeof(float);

static GLuint PreprocessTexture(const char* filepath)
{
    GLuint tex = loadImageToTexture(filepath);
    if (!tex) return 0;

    glBindTexture(GL_TEXTURE_2D, tex);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

bool Renderer::Init(GLuint shaderProgram, const char* overlayPath)
{
    shader = shaderProgram;
    glUseProgram(shader);

    uM = glGetUniformLocation(shader, "uM");
    uV = glGetUniformLocation(shader, "uV");
    uP = glGetUniformLocation(shader, "uP");
    uUseTex = glGetUniformLocation(shader, "useTex");
    uTransparent = glGetUniformLocation(shader, "transparent");
    uTint = glGetUniformLocation(shader, "uTint");
    uTex  = glGetUniformLocation(shader, "uTex");

    glUniform1i(uTex, 0);

    CreateCube();

    overlayTex = PreprocessTexture(overlayPath);

    centerTex = PreprocessTexture("res/center.png");

    std::vector<float> c;
    c.reserve(4 * STRIDE_FLOATS);

    float s = 0.01f; 

    auto pushC = [&](float x, float y, float z, float u, float v){
        c.push_back(x); c.push_back(y); c.push_back(z);
        c.push_back(1); c.push_back(1); c.push_back(1); c.push_back(1);
        c.push_back(u); c.push_back(v);
        c.push_back(0); c.push_back(0); c.push_back(1);
    };

    pushC(-s,  s, 0.0f, 0, 1);
    pushC( s,  s, 0.0f, 1, 1);
    pushC( s, -s, 0.0f, 1, 0);
    pushC(-s, -s, 0.0f, 0, 0);

    CreateFromFloats(centerQuad, c, false);
    centerQuad.vertexCount = 4;


    std::vector<float> q;
    q.reserve(4 * STRIDE_FLOATS);

    auto push = [&](float x, float y, float z, float u, float v){
        q.push_back(x); q.push_back(y); q.push_back(z);
        q.push_back(1); q.push_back(1); q.push_back(1); q.push_back(1);
        q.push_back(u); q.push_back(v);
        q.push_back(0); q.push_back(0); q.push_back(1);
    };

    push(-0.95f, -0.75f, 0.0f, 0, 1);
    push(-0.75f, -0.75f, 0.0f, 1, 1);
    push(-0.75f, -0.95f, 0.0f, 1, 0);
    push(-0.95f, -0.95f, 0.0f, 0, 0);

    CreateFromFloats(overlayQuad, q, false);
    overlayQuad.vertexCount = 4;

    return true;
}

void Renderer::Destroy()
{
    auto kill = [](MeshGL& m){
        if (m.vbo) glDeleteBuffers(1, &m.vbo);
        if (m.vao) glDeleteVertexArrays(1, &m.vao);
        m = MeshGL{};
    };

    kill(cube);
    kill(basin);
    kill(water);
    kill(dropletSphere);
    kill(overlayQuad);
    kill(centerQuad);

    if (overlayTex) glDeleteTextures(1, &overlayTex);
    overlayTex = 0;

    if (centerTex) glDeleteTextures(1, &centerTex);
    centerTex = 0;
}

void Renderer::CreateFromFloats(MeshGL& m, const std::vector<float>& data, bool cubeFans)
{
    if (m.vao) glDeleteVertexArrays(1, &m.vao);
    if (m.vbo) glDeleteBuffers(1, &m.vbo);

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE_BYTES, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, STRIDE_BYTES, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, STRIDE_BYTES, (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, STRIDE_BYTES, (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    m.vertexCount = (GLsizei)(data.size() / STRIDE_FLOATS);
    m.isCubeFans = cubeFans;
}

void Renderer::CreateCube()
{
    float cubeVerts[] =
    {
        // Front
         0.1f, 0.1f, 0.1f,  1,1,1,1,  0,0,   0,0, 1,
        -0.1f, 0.1f, 0.1f,  1,1,1,1,  1,0,   0,0, 1,
        -0.1f,-0.1f, 0.1f,  1,1,1,1,  1,1,   0,0, 1,
         0.1f,-0.1f, 0.1f,  1,1,1,1,  0,1,   0,0, 1,

        // Left
        -0.1f, 0.1f, 0.1f,  1,1,1,1,  0,0,  -1,0,0,
        -0.1f, 0.1f,-0.1f,  1,1,1,1,  1,0,  -1,0,0,
        -0.1f,-0.1f,-0.1f,  1,1,1,1,  1,1,  -1,0,0,
        -0.1f,-0.1f, 0.1f,  1,1,1,1,  0,1,  -1,0,0,

        // Bottom
         0.1f,-0.1f, 0.1f,  1,1,1,1,  0,0,   0,-1,0,
        -0.1f,-0.1f, 0.1f,  1,1,1,1,  1,0,   0,-1,0,
        -0.1f,-0.1f,-0.1f,  1,1,1,1,  1,1,   0,-1,0,
         0.1f,-0.1f,-0.1f,  1,1,1,1,  0,1,   0,-1,0,

        // Top
         0.1f, 0.1f, 0.1f,  1,1,1,1,  0,0,   0,1,0,
         0.1f, 0.1f,-0.1f,  1,1,1,1,  1,0,   0,1,0,
        -0.1f, 0.1f,-0.1f,  1,1,1,1,  1,1,   0,1,0,
        -0.1f, 0.1f, 0.1f,  1,1,1,1,  0,1,   0,1,0,

        // Right
         0.1f, 0.1f, 0.1f,  1,1,1,1,  0,0,   1,0,0,
         0.1f,-0.1f, 0.1f,  1,1,1,1,  1,0,   1,0,0,
         0.1f,-0.1f,-0.1f,  1,1,1,1,  1,1,   1,0,0,
         0.1f, 0.1f,-0.1f,  1,1,1,1,  0,1,   1,0,0,

        // Back
         0.1f, 0.1f,-0.1f,  1,1,1,1,  0,0,  0,0,-1,
         0.1f,-0.1f,-0.1f,  1,1,1,1,  1,0,  0,0,-1,
        -0.1f,-0.1f,-0.1f,  1,1,1,1,  1,1,  0,0,-1,
        -0.1f, 0.1f,-0.1f,  1,1,1,1,  0,1,  0,0,-1,
    };

    std::vector<float> v;
    v.assign(cubeVerts, cubeVerts + (sizeof(cubeVerts) / sizeof(float)));
    CreateFromFloats(cube, v, true);
    cube.vertexCount = 24;
    cube.isCubeFans = true;
}

void Renderer::SetCommonUniforms(const glm::mat4& V, const glm::mat4& P)
{
    glUseProgram(shader);
    glUniformMatrix4fv(uV, 1, GL_FALSE, glm::value_ptr(V));
    glUniformMatrix4fv(uP, 1, GL_FALSE, glm::value_ptr(P));
}

void Renderer::DrawCube(const glm::mat4& M, const glm::vec4& tint, bool transparentFlag)
{
    glUniform1i(uUseTex, 0);
    glUniform1i(uTransparent, transparentFlag ? 1 : 0);
    glUniform4f(uTint, tint.r, tint.g, tint.b, tint.a);
    glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(M));

    glBindVertexArray(cube.vao);
    for (int i = 0; i < 6; ++i)
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    glBindVertexArray(0);
}

void Renderer::DrawMeshTriangles(const MeshGL& m, const glm::mat4& M, const glm::vec4& tint, bool transparentFlag)
{
    glUniform1i(uUseTex, 0);
    glUniform1i(uTransparent, transparentFlag ? 1 : 0);
    glUniform4f(uTint, tint.r, tint.g, tint.b, tint.a);
    glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(M));

    glBindVertexArray(m.vao);
    glDrawArrays(GL_TRIANGLES, 0, m.vertexCount);
    glBindVertexArray(0);
}

void Renderer::DrawOverlay()
{
    glUseProgram(shader);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glm::mat4 I(1.0f);
    glUniformMatrix4fv(uV, 1, GL_FALSE, glm::value_ptr(I));
    glUniformMatrix4fv(uP, 1, GL_FALSE, glm::value_ptr(I));
    glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(I));

    glUniform1i(uUseTex, 1);
    glUniform1i(uTransparent, 1);
    glUniform4f(uTint, 1.0f, 1.0f, 1.0f, 0.4f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, overlayTex);

    glBindVertexArray(overlayQuad.vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawTexturedMesh(const MeshGL& m, const glm::mat4& M, GLuint texID, const glm::vec4& tint)
{
    glUseProgram(shader);

    glUniform1i(uUseTex, 1);
    glUniform1i(uTransparent, 1);
    glUniform4f(uTint, tint.r, tint.g, tint.b, tint.a);
    glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(M));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);

    glBindVertexArray(m.vao);
    glDrawArrays(GL_TRIANGLES, 0, m.vertexCount);
    glBindVertexArray(0);

    glUniform1i(uUseTex, 0);
}

void Renderer::DrawCenter()
{
    glUseProgram(shader);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glm::mat4 I(1.0f);
    glUniformMatrix4fv(uV, 1, GL_FALSE, glm::value_ptr(I));
    glUniformMatrix4fv(uP, 1, GL_FALSE, glm::value_ptr(I));
    glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(I));

    glUniform1i(uUseTex, 1);
    glUniform1i(uTransparent, 1);
    glUniform4f(uTint, 1, 1, 1, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, centerTex);

    glBindVertexArray(centerQuad.vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

#include "../Header/RenderUtils.h"
#include "../Header/stb_image.h"
#include "../Header/Shader.h"
#include <iostream>

unsigned int quadVAO = 0, quadVBO = 0;
extern Shader* basicShader;

void initQuad()
{
    float vertices[] = {
    // positions   // texcoords
     0.0f, 0.0f,   0.0f, 0.0f,
     1.0f, 0.0f,   1.0f, 0.0f,
     1.0f, 1.0f,   1.0f, 1.0f,
     0.0f, 1.0f,   0.0f, 1.0f
};


    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // aPos (location = 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // aTex (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

unsigned int loadTexture(const char* path)
{
    int w, h, ch;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 4);

    if (!data)
    {
        std::cout << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return tex;
}

void drawQuad(float x, float y, float w, float h, unsigned int texture, float alpha)
{
    basicShader->use();
    basicShader->setFloat("alpha", alpha);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(quadVAO);

    // Build transform matrix
    float t[16] = {
        w, 0, 0, 0,
        0, h, 0, 0,
        0, 0, 1, 0,
        x, y, 0, 1
    };

    basicShader->setMat4("transform", t);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

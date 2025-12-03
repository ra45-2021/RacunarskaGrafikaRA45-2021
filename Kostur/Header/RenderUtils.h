#pragma once
#include <GL/glew.h>

void initQuad();
void drawQuad(float x, float y, float w, float h, unsigned int texture, float alpha = 1.0f);
unsigned int loadTexture(const char* path);

extern unsigned int quadVAO;
extern unsigned int quadVBO;

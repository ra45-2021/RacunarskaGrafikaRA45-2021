#pragma once
#include <GL/glew.h>

void initQuad();
unsigned int loadTexture(const char* path);

void drawQuad(float x,float y,float w,float h,
              unsigned int texture,float alpha=1.0f,
              float r=1.0f,float g=1.0f,float b=1.0f);

extern unsigned int quadVAO;
extern unsigned int quadVBO;

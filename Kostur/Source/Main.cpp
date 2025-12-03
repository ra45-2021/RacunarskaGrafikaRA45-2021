#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <unistd.h>

#include "../Header/Globals.h"
#include "../Header/Shader.h"
#include "../Header/RenderUtils.h"

Shader* basicShader = nullptr;

int main()
{
    // --- SETUP ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(mon);

    SCREEN_WIDTH = mode->width;
    SCREEN_HEIGHT = mode->height;

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Klima", mon, NULL);
    glfwMakeContextCurrent(window);
    glewInit();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- SHADER ---
    Shader shader("shaders/basic.vs", "shaders/basic.fs");
    basicShader = &shader;

    // --- GEOMETRY ---
    initQuad();

    // --- TEST WHITE TEXTURE ---
    unsigned int whiteTex;
    unsigned char whitePixel[4] = {255,255,255,255};
    glGenTextures(1, &whiteTex);
    glBindTexture(GL_TEXTURE_2D, whiteTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1,1,0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

    // --- FPS LIMITER ---
    double targetFPS = 1.0 / 75.0;
    double lastTime = glfwGetTime();

    // --- MAIN LOOP ---
    while (!glfwWindowShouldClose(window))
    {
        double now = glfwGetTime();
        double dt = now - lastTime;
        lastTime = now;

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // --- DRAW TEST WHITE RECTANGLE ---
drawQuad(-0.25f, -0.25f, 0.5f, 0.5f, whiteTex, 1.0f);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // FPS limit
        double frame = glfwGetTime() - now;
        if (frame < targetFPS)
            usleep((targetFPS - frame) * 1e6);
    }

    glfwTerminate();
    return 0;
}

// Klima uredjaj (3D) - projekat (podeljen na fajlove)

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Util.h"

#include "Header/Globals.h"
#include "Header/Camera.h"
#include "Header/MeshBuilders.h"
#include "Header/Renderer.h"

// -------------------- HELPERS --------------------
static void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void keyCallback(GLFWwindow* window, int key, int, int action, int)
{
    if (key == GLFW_KEY_G && action == GLFW_PRESS) gUseTex = !gUseTex;
    if (key == GLFW_KEY_T && action == GLFW_PRESS) gTransparent = !gTransparent;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

// ray-sphere intersection (LED + basin pick)
static bool RayHitsSphere(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& c, float r)
{
    glm::vec3 oc = ro - c;
    float b = glm::dot(oc, rd);
    float cterm = glm::dot(oc, oc) - r * r;
    float h = b * b - cterm;
    return h >= 0.0f;
}

// dot gledanja ka klimi (XZ)
static float FacingDotToAC(const glm::vec3& camPos, const glm::vec3& camFront, const glm::vec3& acPos)
{
    glm::vec3 f = glm::normalize(glm::vec3(camFront.x, 0.0f, camFront.z));
    glm::vec3 toAc = glm::normalize(glm::vec3(acPos.x - camPos.x, 0.0f, acPos.z - camPos.z));
    return glm::dot(f, toAc);
}

int main()
{
    if (!glfwInit()) {
        std::cout << "GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    const char wTitle[] = "Klima 3D";

    // FULLSCREEN
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, wTitle, monitor, nullptr);
    if (!window) {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // mouse look
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        glfwTerminate();
        return 3;
    }

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    // required toggles
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // shader
    unsigned int shader = createShader("basic.vert", "basic.frag");
    const int uLightDir = glGetUniformLocation(shader, "uLightDir");
    glUseProgram(shader);
    glUniform3f(uLightDir, -0.4f, -1.0f, -0.2f);

    Renderer R;
    R.Init(shader, "res/overlay.png");

    // -------------------- Create basin / water meshes from builders --------------------
    // kratko: gBasinHeight (default 0.30 in Globals.cpp)
    std::vector<float> basinMesh;
    BuildCylinder(basinMesh, 0.30f, gBasinHeight, 64, glm::vec4(0.70f,0.70f,0.70f,1.0f), true);
    R.CreateFromFloats(R.basin, basinMesh, false);

    std::vector<float> waterMesh;
    BuildCylinder(waterMesh, 0.26f, gBasinHeight, 64, glm::vec4(0.30f,0.60f,1.00f,0.55f), true);
    R.CreateFromFloats(R.water, waterMesh, false);

    // droplets = sphere
    std::vector<float> sphereMesh;
    BuildSphere(sphereMesh, 1.0f, 16, 12, glm::vec4(0.35f,0.70f,1.0f,0.55f));
    R.CreateFromFloats(R.dropletSphere, sphereMesh, false);

    // update derived positions
    gLedPos = gAcPos + glm::vec3(1.35f, 0.20f, 0.55f);

    glClearColor(0.08f, 0.10f, 0.13f, 1.0f);

    // timing
    const double targetFrame = 1.0 / 75.0;
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        const double frameStart = glfwGetTime();
        double dt = frameStart - lastTime;
        lastTime = frameStart;
        if (dt > 0.05) dt = 0.05;

        // required toggles by keys 1-4
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) glEnable(GL_DEPTH_TEST);
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) glDisable(GL_DEPTH_TEST);
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) glEnable(GL_CULL_FACE);
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) glDisable(GL_CULL_FACE);

        // optional move with arrows (planar)
        float speed = 1.6f * (float)dt;
        float fw = 0.0f, rt = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    fw += speed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  fw -= speed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  rt += speed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) rt -= speed;
        if (fw != 0.0f || rt != 0.0f) gCamera.MovePlanar(fw, rt);

        // -------------------- INPUT: LED gaze toggle + basin click --------------------
        bool lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (lmb && !gPrevLmb)
        {
            // gaze + click => toggle AC (block if basinFull is desired - you can keep or remove)
            if (RayHitsSphere(gCamera.pos, gCamera.front, gLedPos, gLedR))
                gAcOn = !gAcOn;

            // click on basin when full and AC off
            if (gBasinFull && !gAcOn && gBasinState == BasinState::OnFloor)
            {
                glm::vec3 basinCenter = gBasinPos;
                if (RayHitsSphere(gCamera.pos, gCamera.front, basinCenter, 0.45f))
                    gBasinState = BasinState::InFrontFull;
            }
        }
        gPrevLmb = lmb;

        // -------------------- cover animation --------------------
        float targetAngle = gAcOn ? -60.0f : 0.0f;
        gCoverAngle = glm::mix(gCoverAngle, targetAngle, 1.0f - pow(0.001f, (float)dt));

        // -------------------- droplets spawn/update + water fill --------------------
        gSpawnAcc += dt;
        if (gAcOn && gBasinState == BasinState::OnFloor && !gBasinFull && gSpawnAcc >= 0.05)
        {
            gSpawnAcc = 0.0;
            Droplet d;
            d.pos = gAcPos + glm::vec3(0.0f, -0.25f, 0.55f);  // nozzle
            d.vel = glm::vec3(0.0f, -0.20f, 0.0f);
            gDrops.push_back(d);
        }

        float halfH = gBasinHeight * 0.5f;
        float basinYTop = gBasinPos.y + halfH;
        float basinYBottom = gBasinPos.y - halfH;

        for (auto& d : gDrops)
        {
            if (!d.alive) continue;

            d.vel.y -= 0.85f * (float)dt;
            d.pos += d.vel * (float)dt;

            glm::vec2 dxz(d.pos.x - gBasinPos.x, d.pos.z - gBasinPos.z);
            bool inside = glm::dot(dxz, dxz) <= gBasinRadiusInner * gBasinRadiusInner;

            float waterY = basinYBottom + (basinYTop - basinYBottom) * gWaterLevel;
            float contactY = (gWaterLevel < 0.02f) ? basinYBottom : waterY;

            if (inside && d.pos.y <= contactY)
            {
                d.alive = false;
                gWaterLevel += 0.02f;

                if (gWaterLevel >= 1.0f) {
                    gWaterLevel = 1.0f;
                    gBasinFull = true;
                    gAcOn = false; // kao u 2D
                }
            }

            if (d.pos.y < -10.0f) d.alive = false;
        }

        // -------------------- basin carry --------------------
        if (gBasinState == BasinState::InFrontFull || gBasinState == BasinState::InFrontEmpty)
        {
            gBasinPos = gCamera.pos + gCamera.front * 0.85f + glm::vec3(0.0f, -0.30f, 0.0f);
        }

        // -------------------- SPACE logic (180 deg / toward AC) --------------------
        bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (space && !gPrevSpace)
        {
            float dotToAc = FacingDotToAC(gCamera.pos, gCamera.front, gAcPos);

            // empty only if ~180 away from AC
            if (gBasinState == BasinState::InFrontFull && dotToAc < -0.90f)
            {
                gWaterLevel = 0.0f;
                gBasinFull = false;
                gBasinState = BasinState::InFrontEmpty;
                for (auto& dr : gDrops) dr.alive = false;
            }
            // return if toward AC
            else if (gBasinState == BasinState::InFrontEmpty && dotToAc > 0.90f)
            {
                gBasinPos = gBasinOriginalPos;
                gBasinState = BasinState::OnFloor;
            }
        }
        gPrevSpace = space;

        // -------------------- Matrices --------------------
        glfwGetFramebufferSize(window, &fbW, &fbH);
        float aspect = (fbH == 0) ? 1.0f : (float)fbW / (float)fbH;

        glm::mat4 V = gCamera.View();
        glm::mat4 P = gCamera.Projection(aspect);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        R.SetCommonUniforms(V, P);

        // -------------------- Render opaque --------------------
        // AC body
        {
            glm::mat4 M(1.0f);
            M = glm::translate(M, gAcPos);
            M = glm::rotate(M, glm::radians(25.0f), glm::vec3(0,1,0));
            M = glm::scale(M, glm::vec3(3.2f, 1.3f, 1.2f));
            R.DrawCube(M, glm::vec4(0.85f, 0.85f, 0.90f, 1.0f), false);
        }

        // cover
        {
            glm::mat4 C(1.0f);
            C = glm::translate(C, gAcPos + glm::vec3(0.0f, 0.25f, 0.60f));
            C = glm::rotate(C, glm::radians(gCoverAngle), glm::vec3(1,0,0));
            C = glm::translate(C, glm::vec3(0.0f, 0.0f, -0.60f));
            C = glm::scale(C, glm::vec3(3.2f, 0.25f, 1.05f));
            R.DrawCube(C, glm::vec4(0.78f, 0.78f, 0.82f, 1.0f), false);
        }

        // LED
        {
            glm::mat4 L(1.0f);
            L = glm::translate(L, gLedPos);
            L = glm::scale(L, glm::vec3(0.18f));
            if (gAcOn) R.DrawCube(L, glm::vec4(0.25f, 1.0f, 0.25f, 1.0f), false);
            else       R.DrawCube(L, glm::vec4(1.0f, 0.25f, 0.25f, 1.0f), false);
        }

        // basin
        {
            glm::mat4 B(1.0f);
            B = glm::translate(B, gBasinPos);
            R.DrawMeshTriangles(R.basin, B, glm::vec4(1,1,1,1), false);
        }

        // -------------------- Transparent: water + droplets --------------------
        // water
        if (gWaterLevel > 0.01f)
        {
            float halfH = gBasinHeight * 0.5f;
            glm::mat4 Wm(1.0f);
            Wm = glm::translate(Wm, gBasinPos + glm::vec3(0.0f, -halfH + halfH * gWaterLevel, 0.0f));
            Wm = glm::scale(Wm, glm::vec3(1.0f, gWaterLevel, 1.0f));
            R.DrawMeshTriangles(R.water, Wm, glm::vec4(1,1,1,1), true);
        }

        // droplets (spheres)
        for (const auto& d : gDrops)
        {
            if (!d.alive) continue;
            glm::mat4 Dm(1.0f);
            Dm = glm::translate(Dm, d.pos);
            Dm = glm::scale(Dm, glm::vec3(0.06f));
            R.DrawMeshTriangles(R.dropletSphere, Dm, glm::vec4(0.35f,0.70f,1.0f,1.0f), true);
        }

        // -------------------- Overlay --------------------
        GLboolean depthWas = glIsEnabled(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST);
        R.DrawOverlay();
        if (depthWas) glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // 75 FPS limiter
        const double frameEnd = glfwGetTime();
        const double frameTime = frameEnd - frameStart;
        if (frameTime < targetFrame)
            std::this_thread::sleep_for(std::chrono::duration<double>(targetFrame - frameTime));
    }

    R.Destroy();
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}

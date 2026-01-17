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

static void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

static void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

static bool RayHitsSphere(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& c, float r) {
    glm::vec3 oc = ro - c;
    float b = glm::dot(oc, rd);
    float cterm = glm::dot(oc, oc) - r * r;
    return (b * b - cterm) >= 0.0f;
}

static float FacingDotToAC(const glm::vec3& camPos, const glm::vec3& camFront, const glm::vec3& acPos) {
    glm::vec3 f = glm::normalize(glm::vec3(camFront.x, 0.0f, camFront.z));
    glm::vec3 toAc = glm::normalize(glm::vec3(acPos.x - camPos.x, 0.0f, acPos.z - camPos.z));
    return glm::dot(f, toAc);
}

int main() {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Klima 3D", monitor, nullptr);
    if (!window) { glfwTerminate(); return 2; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    if (glewInit() != GLEW_OK) { glfwTerminate(); return 3; }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);

    unsigned int shader = createShader("basic.vert", "basic.frag");
    Renderer R;
    R.Init(shader, "res/overlay.png");

    std::vector<float> basinMesh, waterMesh, sphereMesh;
    BuildCylinder(basinMesh, 0.30f, gBasinHeight, 64, glm::vec4(0.412, 0.741, 0.859, 1.0f), true);
    R.CreateFromFloats(R.basin, basinMesh, false);
    BuildCylinder(waterMesh, 0.30f, gBasinHeight, 64, glm::vec4(0.30f, 0.60f, 1.00f, 0.55f), true);
    R.CreateFromFloats(R.water, waterMesh, false);
    BuildSphere(sphereMesh, 1.0f, 16, 12, glm::vec4(0.35f, 0.70f, 1.0f, 0.55f));
    R.CreateFromFloats(R.dropletSphere, sphereMesh, false);

    gLedPos = gAcPos + glm::vec3(0.35f, -0.08f, 0.13f);
    glClearColor(0.671f, 0.851f, 0.89f, 1.0f);

    unsigned int digitTex[10];
    for(int i = 0; i < 10; i++) {
        digitTex[i] = loadImageToTexture(("res/digits/" + std::to_string(i) + ".png").c_str());
    }
    unsigned int fireTex = loadImageToTexture("res/fire.png");
    unsigned int snowTex = loadImageToTexture("res/snow.png");
    unsigned int okTex   = loadImageToTexture("res/check.png");
    unsigned int lampTex = loadImageToTexture("res/lamp.png");
    unsigned int minusTex = loadImageToTexture("res/minus.png");

    unsigned int whiteTex;
    unsigned char px[4] = {255,255,255,255};
    glGenTextures(1, &whiteTex);
    glBindTexture(GL_TEXTURE_2D, whiteTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glBindTexture(GL_TEXTURE_2D, 0);


    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double frameStart = glfwGetTime();
        double dt = frameStart - lastTime;
        lastTime = frameStart;
        if (dt > 0.05) dt = 0.05;

        // Kontrole kretanja (Strelice)
        float speed = 2.0f * (float)dt;
        float fw = 0.0f, rt = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    fw += speed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  fw -= speed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  rt += speed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) rt -= speed;
        if (fw != 0.0f || rt != 0.0f) gCamera.MovePlanar(fw, rt);

        // Kontrola temperature (W i S)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            desiredTemp = std::min(40.0f, desiredTemp + 0.05f);

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            desiredTemp = std::max(-10.0f, desiredTemp - 0.05f);
        // Miš interakcija
        bool lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (lmb && !gPrevLmb) {
            if (RayHitsSphere(gCamera.pos, gCamera.front, gLedPos, gLedR)) {
                if (!gBasinFull) gAcOn = !gAcOn;
                else gAcOn = false;
            }
            if (gBasinFull && !gAcOn && gBasinState == BasinState::OnFloor) {
                if (RayHitsSphere(gCamera.pos, gCamera.front, gBasinPos, 0.45f))
                    gBasinState = BasinState::InFrontFull;
            }
        }
        gPrevLmb = lmb;

        // Space interakcija
        bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (space && !gPrevSpace) {
            float dotToAc = FacingDotToAC(gCamera.pos, gCamera.front, gAcPos);
            if (gBasinState == BasinState::InFrontFull && dotToAc < -0.80f) {
                gWaterLevel = 0.0f;
                gBasinFull = false;
                gBasinState = BasinState::InFrontEmpty;
            } else if (gBasinState == BasinState::InFrontEmpty && dotToAc > 0.80f) {
                gBasinState = BasinState::OnFloor;
            }
        }
        gPrevSpace = space;

        // Animacija klime i vode
        float targetAngle = gAcOn ? 45.0f : 0.0f;
        gCoverAngle = glm::mix(gCoverAngle, targetAngle, 1.0f - pow(0.001f, (float)dt));

        if (gAcOn) {
            float diff = desiredTemp - measuredTemp;
            if (std::abs(diff) > 0.05f) measuredTemp += (diff > 0 ? 1.0f : -1.0f) * (float)dt * 0.9f;
            
            gSpawnAcc += dt;
            if (gBasinState == BasinState::OnFloor && !gBasinFull && gSpawnAcc >= 0.42) {
                gSpawnAcc = 0.0;
                Droplet d;
                d.pos = gAcPos + glm::vec3(0.0f, -0.15f, 0.09f);
                d.vel = glm::vec3(0.0f, -0.05f, 0.0f);
                gDrops.push_back(d);
            }
        }

        for (auto& d : gDrops) {
            if (!d.alive) continue;
            d.vel.y -= 0.25f * (float)dt;
            d.pos += d.vel * (float)dt;
            glm::vec2 dxz(d.pos.x - gBasinPos.x, d.pos.z - gBasinPos.z);
            float waterY = gBasinPos.y - (gBasinHeight * 0.5f) + (gBasinHeight * gWaterLevel);
            if (glm::dot(dxz, dxz) <= gBasinRadiusInner * gBasinRadiusInner && d.pos.y <= waterY) {
                d.alive = false;
                gWaterLevel += 0.02f;
                if (gWaterLevel >= 0.81f) { gAcOn = false; gBasinFull = true; }
            }
        }

        if (gBasinState == BasinState::InFrontFull || gBasinState == BasinState::InFrontEmpty)
            gBasinPos = glm::mix(gBasinPos, gCamera.pos + gCamera.front * 1.5f + glm::vec3(0,-0.3f,0), 0.1f);
        else
            gBasinPos = glm::mix(gBasinPos, gBasinOriginalPos, 0.1f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        int fbW, fbH; glfwGetFramebufferSize(window, &fbW, &fbH);
        R.SetCommonUniforms(gCamera.View(), gCamera.Projection((float)fbW/(float)fbH));
        glUniform3f(glGetUniformLocation(shader, "uLightDir"), -0.4f, -1.0f, -0.2f);

        // Render klime
        glm::mat4 Ma = glm::scale(glm::translate(glm::mat4(1.0f), gAcPos), glm::vec3(4.5f, 1.3f, 1.2f));
        R.DrawCube(Ma, glm::vec4(1.05f, 1.05f, 1.05f, 1.05f), false);

        // COVER 
        glm::vec3 coverSize(4.5f, 0.05f, 1.2f);

        glm::vec3 coverBase = gAcPos + glm::vec3(0.0f, -0.135f, 0.003f);
        float halfD = coverSize.z * 0.1f;

        glm::mat4 Mc(1.0f);

        Mc = glm::translate(Mc, coverBase);
        Mc = glm::translate(Mc, glm::vec3(0.0f, 0.0f, -halfD));
        Mc = glm::rotate(Mc, glm::radians(gCoverAngle), glm::vec3(1.0f,0.0f,0.0f));
        Mc = glm::translate(Mc, glm::vec3(0.0f, 0.0f, +halfD));
        Mc = glm::scale(Mc, coverSize);

        R.DrawCube(Mc, glm::vec4(0.9f, 0.95f, 0.97f, 1.0f), false);

        // LED i Displej
        glm::mat4 Ml = glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), gLedPos), glm::radians(90.0f), glm::vec3(1,0,0)),
            glm::vec3(0.085f, 0.08f, 0.08f)
        );
        R.DrawTexturedMesh(R.basin, Ml, lampTex, gAcOn ? glm::vec4(1, 0.2f, 0.2f, 1) : glm::vec4(0.5f, 0.5f, 0.5f, 1)
        );

        // ---------- Screen rectangles (ALWAYS) ----------
        float pZ = 0.125f;

        // these are the 3 screen “panels” on the AC (positions)
        glm::vec3 s1(-0.30f, -0.075f, pZ);
        glm::vec3 s2(-0.05f, -0.075f, pZ);
        glm::vec3 s3( 0.20f, -0.075f, pZ);

        // size of the panel rectangles
        glm::vec3 screenScale(0.17f, 0.07f, 1.0f);

        // panel tint: grey when OFF, brighter when ON
        glm::vec4 panelTint = gAcOn ? glm::vec4(0.85f,0.85f,0.85f,1.0f)
                                    : glm::vec4(0.55f,0.55f,0.55f,1.0f);

        auto DrawScreenQuad = [&](const glm::vec3& offset, const glm::vec3& S, GLuint tex, const glm::vec4& tint)
        {
            glm::mat4 M = glm::translate(glm::mat4(1.0f), gAcPos + offset);
            M = glm::scale(M, S);
            R.DrawTexturedScreen(M, tex, tint);
        };

        // draw 3 panels
        DrawScreenQuad(s1, screenScale, whiteTex, panelTint);
        DrawScreenQuad(s2, screenScale, whiteTex, panelTint);
        DrawScreenQuad(s3, screenScale, whiteTex, panelTint);

        // ---------- Digits + minus + icon (ALWAYS, like 2D) ----------
        glm::vec4 digitTint = gAcOn ? glm::vec4(0.88f, 1.05f, 1.00f, 1.0f)
                                    : glm::vec4(0.70f, 0.70f, 0.70f, 1.0f);

        // slot size inside the panel
        glm::vec3 slotScale(0.05f, 0.05f, 1.0f);
        glm::vec3 minusScale(0.05f, 0.05f, 1.0f);

        float spacing = 0.01f;
        float rightShift = 0.009f; 

        auto DrawTemperature3D = [&](const glm::vec3& panelCenter, int value)
        {
            float zFront = 0.002f;
            float zBack  = -0.002f;
            float zOffset = gAcOn ? zFront : zBack;

            bool isNegative = value < 0;
            int absValue = std::abs(value);

            int tens = absValue / 10;
            int ones = absValue % 10;

            float wD = slotScale.x;
            float wM = minusScale.x;

            float totalWidth = wM + spacing + wD + spacing + wD; 
            float cursorX = panelCenter.x - totalWidth * 0.5f + rightShift;

            if (isNegative) {
                DrawScreenQuad(glm::vec3(cursorX, panelCenter.y, panelCenter.z + zOffset),
                            minusScale, minusTex, digitTint);
            }
            cursorX += wM + spacing;

            // tens
            DrawScreenQuad(glm::vec3(cursorX, panelCenter.y, panelCenter.z + zOffset),
                        slotScale, digitTex[tens], digitTint);
            cursorX += wD + spacing;

            // ones
            DrawScreenQuad(glm::vec3(cursorX, panelCenter.y, panelCenter.z + zOffset),
                        slotScale, digitTex[ones], digitTint);
        };


        // panel centers (match 2D idea: draw temp centered inside each panel)
        glm::vec3 center1 = s1 + glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 center2 = s2 + glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 center3 = s3 + glm::vec3(0.0f, 0.0f, 0.0f);

        DrawTemperature3D(center1, (int)desiredTemp);
        DrawTemperature3D(center2, (int)measuredTemp);

        // icon in 3rd panel (centered)
        GLuint icon = okTex;
        float iconDiff = desiredTemp - measuredTemp;
        if (std::fabs(iconDiff) < 0.5f) icon = okTex;
        else if (iconDiff > 0.0f) icon = fireTex;
        else icon = snowTex;

        float zFront = 0.002f;
        float zBack  = -0.002f;
        float zOffset = gAcOn ? zFront : zBack;

        glm::vec3 iconScale(0.07f, 0.07f, 1.0f);
        glm::vec3 iconPos = glm::vec3(center3.x, center3.y, center3.z + zOffset);

        DrawScreenQuad(iconPos, iconScale, icon, digitTint);


        // Lavor, voda i kapljice
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        R.DrawMeshTriangles(
            R.basin,
            glm::translate(glm::mat4(1.0f), gBasinPos),
            glm::vec4(1,1,1,1),
            false
        );

        if (gWaterLevel > 0.01f) {
            float waterCenterY = (-gBasinHeight * 0.5f) + (gBasinHeight * gWaterLevel * 0.5f);

            glm::mat4 Mw =
                glm::scale(
                    glm::translate(glm::mat4(1.0f),
                        gBasinPos + glm::vec3(0.0f, waterCenterY, 0.0f)),
                    glm::vec3(0.990f, gWaterLevel, 0.990f)   
                );

            glDepthMask(GL_FALSE);

            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);

            R.DrawMeshTriangles(R.water, Mw, glm::vec4(1,1,1,1), true);

            glDisable(GL_POLYGON_OFFSET_FILL);
            glDepthMask(GL_TRUE);
        }

        glEnable(GL_CULL_FACE);

        for (auto& d : gDrops) if(d.alive) R.DrawMeshTriangles(R.dropletSphere, glm::scale(glm::translate(glm::mat4(1.0f), d.pos), glm::vec3(0.02f)), glm::vec4(0.35f,0.7f,1,1), true);

        R.DrawOverlay();
        R.DrawCenter();

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        double frameTime = glfwGetTime() - frameStart;
        if (frameTime < (1.0/75.0)) std::this_thread::sleep_for(std::chrono::duration<double>((1.0/75.0) - frameTime));
    }

    R.Destroy();
    glDeleteProgram(shader);
    glfwTerminate();
    return 0;
}
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

static void framebuffer_size_callback(GLFWwindow *, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void keyCallback(GLFWwindow *window, int key, int, int action, int)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

static bool RayHitsSphere(const glm::vec3 &ro, const glm::vec3 &rd, const glm::vec3 &c, float r)
{
    glm::vec3 oc = ro - c;

    float a = glm::dot(rd, rd);
    float b = 2.0f * glm::dot(oc, rd);
    float cc = glm::dot(oc, oc) - r * r;

    float discriminant = b * b - 4.0f * a * cc;
    if (discriminant < 0.0f)
        return false;

    float sqrtD = std::sqrt(discriminant);

    float t1 = (-b - sqrtD) / (2.0f * a);
    float t2 = (-b + sqrtD) / (2.0f * a);

    return (t1 > 0.0f) || (t2 > 0.0f);
}

static float FacingDotToAC(const glm::vec3 &camPos, const glm::vec3 &camFront, const glm::vec3 &acPos)
{
    glm::vec3 f = glm::normalize(glm::vec3(camFront.x, 0.0f, camFront.z));
    glm::vec3 toAc = glm::normalize(glm::vec3(acPos.x - camPos.x, 0.0f, acPos.z - camPos.z));
    return glm::dot(f, toAc);
}

int main()
{
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "Klima 3D", monitor, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    if (glewInit() != GLEW_OK)
    {
        glfwTerminate();
        return 3;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);

    unsigned int shader = createShader("basic.vert", "basic.frag");
    Renderer R;
    R.Init(shader, "res/overlay.png");

    std::vector<float> basinMesh, waterMesh, sphereMesh;
    BuildCylinder(basinMesh, 0.30f, gBasinHeight, 64, glm::vec4(0.831f, 0.722f, 0.702f, 1.0f), true);
    R.CreateFromFloats(R.basin, basinMesh, false);
    BuildCylinder(waterMesh, 0.30f, gBasinHeight, 64, glm::vec4(0.30f, 0.60f, 1.00f, 0.55f), true);
    R.CreateFromFloats(R.water, waterMesh, false);
    BuildSphere(sphereMesh, 1.0f, 16, 12, glm::vec4(0.35f, 0.70f, 1.0f, 0.55f));
    R.CreateFromFloats(R.dropletSphere, sphereMesh, false);

    gLedPos = gAcPos + glm::vec3(0.38f, -0.075f, 0.13f);
    glClearColor(0.671f, 0.851f, 0.89f, 1.0f);

    unsigned int overlayTex = loadImageToTexture("res/overlay.png");
    unsigned int wallTex = loadImageToTexture("res/wall.png");
    unsigned int floorTex = loadImageToTexture("res/floor.png");

    unsigned int whiteTex;
    unsigned char px[4] = {255, 255, 255, 255};
    glGenTextures(1, &whiteTex);
    glBindTexture(GL_TEXTURE_2D, whiteTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glBindTexture(GL_TEXTURE_2D, 0);
    unsigned int laptopTopTex = loadImageToTexture("res/laptop-top.png");
    unsigned int laptopBottomTex = loadImageToTexture("res/laptop-bottom.png");
    unsigned int iphoneTex = loadImageToTexture("res/iphone.png");

    unsigned int lampTex = loadImageToTexture("res/lamp.png");
    unsigned int digitTex[10];
    for (int i = 0; i < 10; i++)
    {
        digitTex[i] = loadImageToTexture(("res/digits/" + std::to_string(i) + ".png").c_str());
    }
    unsigned int fireTex = loadImageToTexture("res/fire.png");
    unsigned int snowTex = loadImageToTexture("res/snow.png");
    unsigned int okTex = loadImageToTexture("res/check.png");
    unsigned int minusTex = loadImageToTexture("res/minus.png");

    double lastTime = glfwGetTime();
    bool depthOn = true;
    bool cullOn = true;
    bool prevD = false;
    bool prevC = false;

    while (!glfwWindowShouldClose(window))
    {
        double frameStart = glfwGetTime();
        double dt = frameStart - lastTime;
        lastTime = frameStart;
        if (dt > 0.05)
            dt = 0.05;

        // Kontrole kretanja (Strelice)
        float speed = 2.0f * (float)dt;
        float fw = 0.0f, rt = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            fw += speed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            fw -= speed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            rt += speed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            rt -= speed;
        if (fw != 0.0f || rt != 0.0f)
            gCamera.MovePlanar(fw, rt);

        // Kontrola temperature (W i S)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            desiredTemp = std::min(40.0f, desiredTemp + 0.05f);

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            desiredTemp = std::max(-10.0f, desiredTemp - 0.05f);

        // Kontrola dubine i odstranjanje naličja (D i C)
        bool dNow = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        if (dNow && !prevD)
            depthOn = !depthOn;
        prevD = dNow;

        bool cNow = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
        if (cNow && !prevC)
            cullOn = !cullOn;
        prevC = cNow;

        depthOn ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        cullOn ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);

        // Kontrola mišem
        {
            glm::vec3 rd = glm::normalize(gCamera.front);

            bool lookLed = RayHitsSphere(gCamera.pos, rd, gLedPos, gLedR);

            if (lookLed && !gPrevLookLed)
            {
                if (!gBasinFull)
                    gAcOn = !gAcOn;
                else
                    gAcOn = false;
            }

            gPrevLookLed = lookLed;

            bool lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            if (lmb && !gPrevLmb)
            {
                if (gBasinFull && !gAcOn && gBasinState == BasinState::OnFloor)
                {
                    if (RayHitsSphere(gCamera.pos, rd, gBasinPos, 0.45f))
                        gBasinState = BasinState::InFrontFull;
                }
            }
            gPrevLmb = lmb;
        }

        // Kontrola space-om
        bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (space && !gPrevSpace)
        {
            float dotToAc = FacingDotToAC(gCamera.pos, gCamera.front, gAcPos);
            if (gBasinState == BasinState::InFrontFull && dotToAc < -0.80f)
            {
                gWaterLevel = 0.0f;
                gBasinFull = false;
                gBasinState = BasinState::InFrontEmpty;
            }
            else if (gBasinState == BasinState::InFrontEmpty && dotToAc > 0.80f)
            {
                gBasinState = BasinState::OnFloor;
            }
        }
        gPrevSpace = space;


        // Animacija klime i vode
        float targetAngle = gAcOn ? 45.0f : 0.0f;
        gCoverAngle = glm::mix(gCoverAngle, targetAngle, 1.0f - pow(0.001f, (float)dt));

        if (gAcOn)
        {
            float diff = desiredTemp - measuredTemp;
            if (std::abs(diff) > 0.05f)
                measuredTemp += (diff > 0 ? 1.0f : -1.0f) * (float)dt * 0.9f;

            gSpawnAcc += dt;
            if (gBasinState == BasinState::OnFloor && !gBasinFull && gSpawnAcc >= 0.42)
            {
                gSpawnAcc = 0.0;
                Droplet d;
                d.pos = gAcPos + glm::vec3(0.0f, -0.15f, 0.09f);
                d.vel = glm::vec3(0.0f, -0.05f, 0.0f);
                gDrops.push_back(d);
            }
        }

        for (auto &d : gDrops)
        {
            if (!d.alive)
                continue;
            d.vel.y -= 0.25f * (float)dt;
            d.pos += d.vel * (float)dt;
            glm::vec2 dxz(d.pos.x - gBasinPos.x, d.pos.z - gBasinPos.z);
            float waterY = gBasinPos.y - (gBasinHeight * 0.5f) + (gBasinHeight * gWaterLevel);
            if (glm::dot(dxz, dxz) <= gBasinRadiusInner * gBasinRadiusInner && d.pos.y <= waterY)
            {
                d.alive = false;
                gWaterLevel += 0.02f;
                if (gWaterLevel >= 0.81f)
                {
                    gAcOn = false;
                    gBasinFull = true;
                }
            }
        }

        if (gBasinState == BasinState::InFrontFull || gBasinState == BasinState::InFrontEmpty)
            gBasinPos = glm::mix(gBasinPos, gCamera.pos + gCamera.front * 1.5f + glm::vec3(0, -0.3f, 0), 0.1f);
        else
            gBasinPos = glm::mix(gBasinPos, gBasinOriginalPos, 0.1f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        R.SetCommonUniforms(gCamera.View(), gCamera.Projection((float)fbW / (float)fbH));
        glUniform3f(glGetUniformLocation(shader, "uLightDir"), -0.4f, -1.0f, -0.2f);

        auto DrawBox = [&](const glm::vec3 &pos, const glm::vec3 &size, const glm::vec4 &col, float rotYdeg = 0.0f)
        {
            glm::mat4 M = glm::translate(glm::mat4(1.0f), pos);
            if (rotYdeg != 0.0f)
                M = glm::rotate(M, glm::radians(rotYdeg), glm::vec3(0, 1, 0));
            M = glm::scale(M, size);
            R.DrawCube(M, col, false);
        };

        //------------------------------------------------Renderovanje------------------------------------------------

        //Zid
        glm::vec3 wallSize(19.0f, 13.0f, 0.1f);
        float acHalfDepth = 1.2f * 0.5f;
        float wallGap = 0.0f;
        glm::vec3 wallPos = gAcPos + glm::vec3(0.0f, -0.85f, -0.12f);

        glm::mat4 Mwall = glm::translate(glm::mat4(1.0f), wallPos);
        Mwall = glm::rotate(Mwall, glm::radians(180.0f), glm::vec3(0, 0, 1));
        Mwall = glm::scale(Mwall, wallSize);

        glDisable(GL_CULL_FACE);
        R.DrawTexturedCube(Mwall, wallTex);
        glEnable(GL_CULL_FACE);

        //Pod
        glm::vec3 floorSize(18.9f, 0.1f, 15.0f);
        float floorY = (gBasinOriginalPos.y - (gBasinHeight * 0.5f)) - (floorSize.y * 0.5f) - 0.02f;
        glm::vec3 floorPos(-0.2f, floorY, -1.62f);

        glm::mat4 Mfloor = glm::translate(glm::mat4(1.0f), floorPos);
        Mfloor = glm::rotate(Mfloor, glm::radians(180.0f), glm::vec3(0, 0, 1));
        Mfloor = glm::scale(Mfloor, floorSize);

        R.DrawTexturedCube(Mfloor, floorTex);

        //------------------------------------------------Nameštaj------------------------------------------------

        //Prvi sto
        glm::vec3 deskTopPos = glm::vec3(-1.305f, -0.60f, -0.65f);
        glm::vec3 deskTopSize = glm::vec3(5.2f, 0.12f, 4.1f);
        DrawBox(deskTopPos, deskTopSize, glm::vec4(0.467f, 0.553f, 0.6f, 1.0f));

        glm::vec3 deskLegSize = glm::vec3(0.12f, 2.35f, 0.12f);

        glm::vec3 deskLeg1Pos = glm::vec3(-1.80f, -0.85f, -0.3f);
        glm::vec3 deskLeg2Pos = glm::vec3(-0.80f, -0.85f, -0.3f);
        glm::vec3 deskLeg3Pos = glm::vec3(-1.80f, -0.85f, -1.0f);
        glm::vec3 deskLeg4Pos = glm::vec3(-0.80f, -0.85f, -1.0f);

        DrawBox(deskLeg1Pos, deskLegSize, glm::vec4(0.20f, 0.20f, 0.20f, 1));
        DrawBox(deskLeg2Pos, deskLegSize, glm::vec4(0.20f, 0.20f, 0.20f, 1));
        DrawBox(deskLeg3Pos, deskLegSize, glm::vec4(0.20f, 0.20f, 0.20f, 1));
        DrawBox(deskLeg4Pos, deskLegSize, glm::vec4(0.20f, 0.20f, 0.20f, 1));

        //Prva stolica
        glm::vec3 chairSeatPos = glm::vec3(-1.296f, -0.82f, -1.03f);
        glm::vec3 chairSeatSize = glm::vec3(1.65f, 0.20f, 1.9f);
        DrawBox(chairSeatPos, chairSeatSize, glm::vec4(0.25f, 0.25f, 0.28f, 1));

        glm::vec3 chairLegSize = glm::vec3(0.11f, 1.55f, 0.11f);

        glm::vec3 chairLeg1Pos = glm::vec3(-1.40f, -1.0f, -0.9f);
        glm::vec3 chairLeg2Pos = glm::vec3(-1.20f, -1.0f, -0.9f);
        glm::vec3 chairLeg3Pos = glm::vec3(-1.40f, -1.0f, -1.17f);
        glm::vec3 chairLeg4Pos = glm::vec3(-1.20f, -1.0f, -1.17f);

        DrawBox(chairLeg1Pos, chairLegSize, glm::vec4(0.12f, 0.12f, 0.12f, 1));
        DrawBox(chairLeg2Pos, chairLegSize, glm::vec4(0.12f, 0.12f, 0.12f, 1));
        DrawBox(chairLeg3Pos, chairLegSize, glm::vec4(0.12f, 0.12f, 0.12f, 1));
        DrawBox(chairLeg4Pos, chairLegSize, glm::vec4(0.12f, 0.12f, 0.12f, 1));

        glm::vec3 chairBackPos = glm::vec3(-1.296f, -0.65f, -1.22f);
        glm::vec3 chairBackSize = glm::vec3(1.65f, 1.9f, 0.25f);
        DrawBox(chairBackPos, chairBackSize, glm::vec4(0.22f, 0.22f, 0.25f, 1));

        
        //Drugi sto
        float dx = 2.2f;
        glm::vec3 shift(dx, 0.0f, 0.0f);

        DrawBox(deskTopPos + shift, deskTopSize, glm::vec4(0.467f, 0.553f, 0.6f, 1.0f));

        DrawBox(deskLeg1Pos + shift, deskLegSize, glm::vec4(0.20f, 0.20f, 0.20f, 1));
        DrawBox(deskLeg2Pos + shift, deskLegSize, glm::vec4(0.20f, 0.20f, 0.20f, 1));
        DrawBox(deskLeg3Pos + shift, deskLegSize, glm::vec4(0.20f, 0.20f, 0.20f, 1));
        DrawBox(deskLeg4Pos + shift, deskLegSize, glm::vec4(0.20f, 0.20f, 0.20f, 1));

        //Druga stolica
        DrawBox(chairSeatPos + shift, chairSeatSize, glm::vec4(0.25f, 0.25f, 0.28f, 1));
        DrawBox(chairBackPos + shift, chairBackSize, glm::vec4(0.22f, 0.22f, 0.25f, 1));

        DrawBox(chairLeg1Pos + shift, chairLegSize, glm::vec4(0.12f, 0.12f, 0.12f, 1));
        DrawBox(chairLeg2Pos + shift, chairLegSize, glm::vec4(0.12f, 0.12f, 0.12f, 1));
        DrawBox(chairLeg3Pos + shift, chairLegSize, glm::vec4(0.12f, 0.12f, 0.12f, 1));
        DrawBox(chairLeg4Pos + shift, chairLegSize, glm::vec4(0.12f, 0.12f, 0.12f, 1));
        

        //------------------------------------------------Stvari na stolovima------------------------------------------------
        
        //Laptop - ekran
        {
            glm::vec3 pos = glm::vec3(-1.3f, -0.441f, -0.579f);
            glm::vec3 size = glm::vec3(0.04f, 1.25f, 1.05f);

            glm::mat4 M = glm::translate(glm::mat4(1.0f), pos);
            M = glm::rotate(M, glm::radians(90.0f), glm::vec3(0, 1, 0));
            M = glm::rotate(M, glm::radians(90.0f), glm::vec3(1, 0, 0));
            M = glm::scale(M, size);

            R.DrawTexturedCubeFace(M, laptopTopTex, glm::vec4(0.984f, 0.855f, 0.835f, 1.0f), glm::vec4(1, 1, 1, 1), CubeFace::Right, 0.004f);
        }

        //Laptop - tastatura
        {
            glm::vec3 pos = glm::vec3(-1.3f, -0.55f, -0.68f);
            glm::vec3 size = glm::vec3(1.05f, 0.04f, 1.25f);

            glm::mat4 M = glm::translate(glm::mat4(1.0f), pos);
            M = glm::rotate(M, glm::radians(90.0f), glm::vec3(0, 1, 0));
            M = glm::scale(M, size);

            R.DrawTexturedCubeFace(M, laptopBottomTex, glm::vec4(0.984f, 0.855f, 0.835f, 1.0f), glm::vec4(1, 1, 1, 1), CubeFace::Top, 0.004f);
        }

        //Papir + Ime
        {
            glm::vec3 pos = glm::vec3(0.9f, -0.57f, -0.65f);
            glm::vec3 size = glm::vec3(1.85f, 0.02f, 1.30f);

            glm::mat4 M = glm::translate(glm::mat4(1.0f), pos);
            M = glm::rotate(M, glm::radians(90.0f), glm::vec3(0, 1, 0));
            M = glm::scale(M, size);

            R.DrawTexturedCubeFace(M, overlayTex, glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 1, 1), CubeFace::Top, 0.004f);
        }

        //Telefon
        {
            glm::vec3 pos = glm::vec3(0.7f, -0.57f, -0.65f);
            glm::vec3 size = glm::vec3(0.64f, 0.03f, 0.28f);

            glm::mat4 M = glm::translate(glm::mat4(1.0f), pos);
            M = glm::rotate(M, glm::radians(90.0f), glm::vec3(0, 1, 0));
            M = glm::scale(M, size);

            R.DrawTexturedCubeFace(M, iphoneTex, glm::vec4(0, 0, 0, 1), glm::vec4(1, 1, 1, 1), CubeFace::Top, 0.003f);
        }

        //------------------------------------------------Glavni deo------------------------------------------------


        //Klima
        glUniform1i(glGetUniformLocation(shader, "uUnlit"), 1);
        glm::mat4 Ma = glm::scale(glm::translate(glm::mat4(1.0f), gAcPos), glm::vec3(4.5f, 1.3f, 1.2f));
        R.DrawCube(Ma, glm::vec4(1.0f, 0.99f, 0.99f, 1.0f), false);
        glUniform1i(glGetUniformLocation(shader, "uUnlit"), 0);


        //Poklopac
        glm::vec3 coverSize(4.5f, 0.05f, 1.2f);
        glm::vec3 coverBase = gAcPos + glm::vec3(0.0f, -0.135f, 0.003f);
        float halfD = coverSize.z * 0.1f;
        glm::mat4 Mc(1.0f);

        Mc = glm::translate(Mc, coverBase);
        Mc = glm::translate(Mc, glm::vec3(0.0f, 0.0f, -halfD));
        Mc = glm::rotate(Mc, glm::radians(gCoverAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        Mc = glm::translate(Mc, glm::vec3(0.0f, 0.0f, +halfD));
        Mc = glm::scale(Mc, coverSize);

        R.DrawCube(Mc, glm::vec4(0.9f, 0.95f, 0.97f, 1.0f), false);


        //Lampica
        glm::mat4 Ml = glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), gLedPos), glm::radians(90.0f), glm::vec3(1, 0, 0)),
            glm::vec3(0.085f, 0.08f, 0.08f));

        R.DrawTexturedMesh(R.basin, Ml, lampTex, gAcOn ? glm::vec4(0.141f, 0.8f, 0.263f, 1.0f) : glm::vec4(0.8787f, 0.204f, 0.051f, 1.0f));
        glDisable(GL_CULL_FACE);


        //Ekrani
        float pZ = 0.126f;

        glm::vec3 s1(-0.30f, -0.075f, pZ);
        glm::vec3 s2(-0.05f, -0.075f, pZ);
        glm::vec3 s3(0.20f, -0.075f, pZ);

        glm::vec3 screenScale(0.19f, 0.07f, 0.99f);
        glm::vec4 panelTint = gAcOn ? glm::vec4(0.922f, 0.922f, 0.922f, 1.0f)
                                    : glm::vec4(0.859f, 0.918f, 0.941f, 1.0f);

        auto DrawScreenQuad = [&](const glm::vec3 &offset, const glm::vec3 &S, GLuint tex, const glm::vec4 &tint)
        {
            glm::mat4 M = glm::translate(glm::mat4(1.0f), gAcPos + offset);
            M = glm::scale(M, S);
            R.DrawTexturedScreen(M, tex, tint);
        };
        DrawScreenQuad(s1, screenScale, whiteTex, panelTint);
        DrawScreenQuad(s2, screenScale, whiteTex, panelTint);
        DrawScreenQuad(s3, screenScale, whiteTex, panelTint);

        //Cifre + Minus
        glm::vec4 digitTint = gAcOn ? glm::vec4(0.88f, 1.05f, 1.00f, 1.0f)
                                    : glm::vec4(0.70f, 0.70f, 0.70f, 1.0f);

        glm::vec3 slotScale(0.06f, 0.06f, 0.0f);
        glm::vec3 minusScale(0.06f, 0.06f, 0.0f);

        float spacing = 0.01f;
        float rightShift = 0.019f;

        auto DrawTemperature3D = [&](const glm::vec3 &panelCenter, int value)
        {
            float zFront = 0.001f;
            float zBack = -0.01f;
            float zOffset = gAcOn ? zFront : zBack;

            bool isNegative = value < 0;
            int absValue = std::abs(value);

            int tens = absValue / 10;
            int ones = absValue % 10;

            float wD = slotScale.x;
            float wM = minusScale.x;

            float totalWidth = wM + spacing + wD + spacing + wD;
            float cursorX = panelCenter.x - totalWidth * 0.5f + rightShift;

            if (isNegative)
            {
                DrawScreenQuad(glm::vec3(cursorX, panelCenter.y, panelCenter.z + zOffset),
                               minusScale, minusTex, digitTint);
            }
            cursorX += wM + spacing;

            DrawScreenQuad(glm::vec3(cursorX, panelCenter.y, panelCenter.z + zOffset),
                           slotScale, digitTex[tens], digitTint);
            cursorX += wD + spacing;

            DrawScreenQuad(glm::vec3(cursorX, panelCenter.y, panelCenter.z + zOffset),
                           slotScale, digitTex[ones], digitTint);
        };

        glm::vec3 center1 = s1 + glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 center2 = s2 + glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 center3 = s3 + glm::vec3(0.0f, 0.0f, 0.0f);

        DrawTemperature3D(center1, (int)desiredTemp);
        DrawTemperature3D(center2, (int)measuredTemp);


        //Ikonice
        GLuint icon = okTex;
        float iconDiff = desiredTemp - measuredTemp;
        if (std::fabs(iconDiff) < 0.5f)
            icon = okTex;
        else if (iconDiff > 0.0f)
            icon = fireTex;
        else
            icon = snowTex;

        float zFront = 0.002f;
        float zBack = -0.002f;
        float zOffset = gAcOn ? zFront : zBack;

        glm::vec3 iconScale(0.075f, 0.075f, 1.0f);
        glm::vec3 iconPos = glm::vec3(center3.x, center3.y, center3.z + zOffset);

        DrawScreenQuad(iconPos, iconScale, icon, digitTint);
        glEnable(GL_CULL_FACE);


        //Lavor + Voda + Kapljice
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        R.DrawMeshTriangles(
            R.basin,
            glm::translate(glm::mat4(1.0f), gBasinPos),
            glm::vec4(1, 1, 1, 1),
            false);

        if (gWaterLevel > 0.01f)
        {
            float waterCenterY = (-gBasinHeight * 0.5f) + (gBasinHeight * gWaterLevel * 0.5f);

            glm::mat4 Mw =
                glm::scale(
                    glm::translate(glm::mat4(1.0f),
                                   gBasinPos + glm::vec3(0.0f, waterCenterY, 0.0f)),
                    glm::vec3(0.990f, gWaterLevel, 0.990f));

            glDepthMask(GL_FALSE);

            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);

            R.DrawMeshTriangles(R.water, Mw, glm::vec4(1, 1, 1, 1), true);

            glDisable(GL_POLYGON_OFFSET_FILL);
            glDepthMask(GL_TRUE);
        }

        glEnable(GL_CULL_FACE);

        for (auto &d : gDrops)
            if (d.alive)
                R.DrawMeshTriangles(R.dropletSphere, glm::scale(glm::translate(glm::mat4(1.0f), d.pos), glm::vec3(0.02f)), glm::vec4(0.35f, 0.7f, 1, 1), true);

        // R.DrawOverlay();
        R.DrawCenter();

        glfwSwapBuffers(window);
        glfwPollEvents();

        double frameTime = glfwGetTime() - frameStart;
        if (frameTime < (1.0 / 75.0))
            std::this_thread::sleep_for(std::chrono::duration<double>((1.0 / 75.0) - frameTime));
    }

    R.Destroy();
    glDeleteProgram(shader);
    glfwTerminate();
    return 0;
}
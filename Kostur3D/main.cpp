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
#include "Header/tiny_obj_loader.h"


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

static bool LoadObjToMeshGL(const char* objPath, Renderer& R, MeshGL& outMesh, const glm::vec4& color)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string baseDir = std::string(objPath);
    auto slash = baseDir.find_last_of("/\\");
    baseDir = (slash == std::string::npos) ? "" : baseDir.substr(0, slash + 1);

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath, baseDir.c_str(), true);
    if (!ok) return false;

    std::vector<float> data;
    data.reserve(12 * 200000);

    auto pushV = [&](float px,float py,float pz, float u,float v, float nx,float ny,float nz)
    {
        data.push_back(px); data.push_back(py); data.push_back(pz);
        data.push_back(color.r); data.push_back(color.g); data.push_back(color.b); data.push_back(color.a);
        data.push_back(u); data.push_back(v);
        data.push_back(nx); data.push_back(ny); data.push_back(nz);
    };

    for (const auto& s : shapes)
    {
        for (const auto& idx : s.mesh.indices)
        {
            float px = attrib.vertices[3 * idx.vertex_index + 0];
            float py = attrib.vertices[3 * idx.vertex_index + 1];
            float pz = attrib.vertices[3 * idx.vertex_index + 2];

            float nx = 0, ny = 1, nz = 0;
            if (idx.normal_index >= 0)
            {
                nx = attrib.normals[3 * idx.normal_index + 0];
                ny = attrib.normals[3 * idx.normal_index + 1];
                nz = attrib.normals[3 * idx.normal_index + 2];
            }

            float u = 0, v = 0;
            if (idx.texcoord_index >= 0)
            {
                u = attrib.texcoords[2 * idx.texcoord_index + 0];
                v = attrib.texcoords[2 * idx.texcoord_index + 1];
            }

            pushV(px, py, pz, u, v, nx, ny, nz);
        }
    }

    R.CreateFromFloats(outMesh, data, false);
    return true;
}

static bool LoadObjToFourMeshesByMtlName(
    const char* objPath, Renderer& R,
    MeshGL& out0, MeshGL& out1, MeshGL& out2, MeshGL& out3,
    const glm::vec4& color,
    const std::string& n0="mat0",
    const std::string& n1="mat1",
    const std::string& n2="mat2",
    const std::string& n3="mat3")
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string baseDir = std::string(objPath);
    auto slash = baseDir.find_last_of("/\\");
    baseDir = (slash == std::string::npos) ? "" : baseDir.substr(0, slash + 1);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath, baseDir.c_str(), true))
        return false;

    int id0=-1,id1=-1,id2=-1,id3=-1;
    for(int i=0;i<(int)materials.size();i++){
        if(materials[i].name==n0) id0=i;
        if(materials[i].name==n1) id1=i;
        if(materials[i].name==n2) id2=i;
        if(materials[i].name==n3) id3=i;
    }

    std::vector<float> d0,d1,d2,d3;
    d0.reserve(12*50000); d1.reserve(12*50000);
    d2.reserve(12*50000); d3.reserve(12*50000);

    auto pushV = [&](std::vector<float>& data, float px,float py,float pz, float u,float v, float nx,float ny,float nz)
    {
        data.push_back(px); data.push_back(py); data.push_back(pz);
        data.push_back(color.r); data.push_back(color.g); data.push_back(color.b); data.push_back(color.a);
        data.push_back(u); data.push_back(v);
        data.push_back(nx); data.push_back(ny); data.push_back(nz);
    };

    for (const auto& s : shapes)
    {
        size_t index_offset = 0;
        for (size_t f = 0; f < s.mesh.num_face_vertices.size(); f++)
        {
            int fv = s.mesh.num_face_vertices[f];
            int faceMat = (f < s.mesh.material_ids.size()) ? s.mesh.material_ids[f] : -1;

            std::vector<float>* target = nullptr;
            if      (faceMat == id0) target = &d0;
            else if (faceMat == id1) target = &d1;
            else if (faceMat == id2) target = &d2;
            else if (faceMat == id3) target = &d3;
            else { index_offset += fv; continue; }

            for (int vtx = 0; vtx < fv; vtx++)
            {
                tinyobj::index_t idx = s.mesh.indices[index_offset + vtx];

                float px = attrib.vertices[3 * idx.vertex_index + 0];
                float py = attrib.vertices[3 * idx.vertex_index + 1];
                float pz = attrib.vertices[3 * idx.vertex_index + 2];

                float nx=0, ny=1, nz=0;
                if (idx.normal_index >= 0){
                    nx = attrib.normals[3 * idx.normal_index + 0];
                    ny = attrib.normals[3 * idx.normal_index + 1];
                    nz = attrib.normals[3 * idx.normal_index + 2];
                }

                float u=0, v=0;
                if (idx.texcoord_index >= 0){
                    u = attrib.texcoords[2 * idx.texcoord_index + 0];
                    v = attrib.texcoords[2 * idx.texcoord_index + 1];
                }

                pushV(*target, px,py,pz, u,v, nx,ny,nz);
            }
            index_offset += fv;
        }
    }

    bool ok=false;
    if(!d0.empty()){ R.CreateFromFloats(out0, d0, false); ok=true; }
    if(!d1.empty()){ R.CreateFromFloats(out1, d1, false); ok=true; }
    if(!d2.empty()){ R.CreateFromFloats(out2, d2, false); ok=true; }
    if(!d3.empty()){ R.CreateFromFloats(out3, d3, false); ok=true; }
    return ok;
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
    unsigned int wall2Tex = loadImageToTexture("res/wall2.png");
    unsigned int floorTex = loadImageToTexture("res/floor.png");
    unsigned int bathroomFloorTex = loadImageToTexture("res/bathroom-floor.png");
    unsigned int bathroomWallTex = loadImageToTexture("res/bathroom-wall.png");

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
    unsigned int toiletTex = loadImageToTexture("res/toilet/Toilet.jpg");
    unsigned int cupboardTex = loadImageToTexture("res/cupboard.png");
    unsigned int remoteTex14 = loadImageToTexture("res/remote/blinn14SG_baseColor.png");
    unsigned int remoteTex8  = loadImageToTexture("res/remote/blinn8SG_baseColor.png");

    MeshGL toiletMesh;
    MeshGL floorMatMesh;
    MeshGL sinkMesh;
    MeshGL remoteMat0Mesh, remoteMat1Mesh, remoteMat2Mesh, remoteMat3Mesh;
    bool toiletOk = LoadObjToMeshGL("res/toilet/10778_Toilet_V2.obj", R, toiletMesh, glm::vec4(1,1,1,1));    
    bool floorMatOk = LoadObjToMeshGL("res/mat/mat.obj", R, floorMatMesh, glm::vec4(0.467f, 0.553f, 0.6f, 1.0f));
    bool sinkOk = LoadObjToMeshGL("res/sink/lavandino.obj", R, sinkMesh, glm::vec4(0.95f, 0.95f, 0.98f, 1.0f));
    bool remoteOk = LoadObjToFourMeshesByMtlName("res/remote/ac_remote__free.obj", R, remoteMat0Mesh, remoteMat1Mesh, remoteMat2Mesh, remoteMat3Mesh, glm::vec4(1,1,1,1), "mat0","mat1","mat2","mat3");

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
bool dNow = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
if (dNow && !prevD) depthOn = !depthOn;
prevD = dNow;

bool cNow = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
if (cNow && !prevC) cullOn = !cullOn;
prevC = cNow;

// Globalno stanje (po frame-u)
depthOn ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
cullOn  ? glEnable(GL_CULL_FACE)  : glDisable(GL_CULL_FACE);

// Helper za “lokalno” menjanje, ali da user-toggle uvek pobedi
auto SetCullLocal = [&](bool wantCullForThisDraw)
{
    if (!cullOn) { glDisable(GL_CULL_FACE); return; } // ako je user ugasio, ne sme niko da ga upali
    wantCullForThisDraw ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
};
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

        glm::vec3 lightPos = glm::vec3(-0.5f, 4.5f, -1.0f);
        glm::vec3 lightColor = glm::vec3(0.98f, 0.98f, 1.0f); 
        float lightPower = 0.8f;  
        float ambient = 0.75f;

        glUniform3fv(glGetUniformLocation(shader, "uLightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shader, "uLightColor"), 1, glm::value_ptr(lightColor));
        glUniform1f(glGetUniformLocation(shader, "uLightPower"), lightPower);
        glUniform1f(glGetUniformLocation(shader, "uAmbient"), ambient);
        glUniform1f(glGetUniformLocation(shader, "uEmissive"), 0.0f);

        glm::mat4 Mlight = glm::scale(glm::translate(glm::mat4(1.0f), lightPos), glm::vec3(0.12f));
        glUniform1i(glGetUniformLocation(shader, "uUnlit"), 1);
        glUniform1f(glGetUniformLocation(shader, "uEmissive"), 1.0f);
        R.DrawMeshTriangles(R.dropletSphere, Mlight, glm::vec4(lightColor, 1.0f), false);

        glUniform1f(glGetUniformLocation(shader, "uEmissive"), 0.0f);
        glUniform1i(glGetUniformLocation(shader, "uUnlit"), 0);

        glm::vec3 lightPos2   = gLedPos;                      
        glm::vec3 lightColor2 = glm::vec3(1.0f, 0.12f, 0.08f);  
        float lightPower2     = gAcOn ? 0.05f : 0.0f;         

        glUniform3fv(glGetUniformLocation(shader, "uLightPos2"), 1, glm::value_ptr(lightPos2));
        glUniform3fv(glGetUniformLocation(shader, "uLightColor2"), 1, glm::value_ptr(lightColor2));
        glUniform1f (glGetUniformLocation(shader, "uLightPower2"), lightPower2);



        auto DrawBox = [&](const glm::vec3 &pos, const glm::vec3 &size, const glm::vec4 &col, float rotYdeg = 0.0f)
        {
            glm::mat4 M = glm::translate(glm::mat4(1.0f), pos);
            if (rotYdeg != 0.0f)
                M = glm::rotate(M, glm::radians(rotYdeg), glm::vec3(0, 1, 0));
            M = glm::scale(M, size);
            R.DrawCube(M, col, false);
        };

        //------------------------------------------------Renderovanje------------------------------------------------

        //Zid 1
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

        // Zid kupatila
        glm::vec3 wall2Size(13.0f, 8.0f, 0.1f);
        glm::vec3 wall2Pos(0.89f, 0.15f, 3.08f);
  
        glm::mat4 Mwall2 = glm::translate(glm::mat4(1.0f), wall2Pos);
        Mwall2 = glm::rotate(Mwall2, glm::radians(270.0f), glm::vec3(0, 0, 1));
        Mwall2 = glm::scale(Mwall2, wall2Size);

        // Zid 2
        glm::vec3 wallThin(0.1f, 13.0f, 20.0f); 

        {
            glm::vec3 pos = glm::vec3(1.7f, 0.15f, -1.13f);
            glm::mat4 M = glm::translate(glm::mat4(1.0f), pos);
            M = glm::scale(M, wallThin);
            glDisable(GL_CULL_FACE);
            R.DrawTexturedCube(M, wall2Tex);
            R.DrawTexturedCube(Mwall2, bathroomWallTex);
            glEnable(GL_CULL_FACE);
        }

        // Zid kupatila 2
        {
            glm::vec3 backWallSize(0.1f, 13.0f, 11.0f);  
            glm::vec3 pos = glm::vec3(1.7f, 0.15f, 1.97f);

            glm::mat4 M = glm::translate(glm::mat4(1.0f), pos);
            M = glm::rotate(M, glm::radians(180.0f), glm::vec3(1, 0, 0));
            M = glm::scale(M, backWallSize);

            glDisable(GL_CULL_FACE);
            R.DrawTexturedCube(M, bathroomWallTex);
            glEnable(GL_CULL_FACE);
        }

        //Pod
        glm::vec3 floorSize(18.9f, 0.1f, 20.0f);
        float floorY = (gBasinOriginalPos.y - (gBasinHeight * 0.5f)) - (floorSize.y * 0.5f) - 0.02f;
        glm::vec3 floorPos(-0.2f, floorY, -1.12f);

        glm::mat4 Mfloor = glm::translate(glm::mat4(1.0f), floorPos);
        Mfloor = glm::rotate(Mfloor, glm::radians(180.0f), glm::vec3(0, 0, 1));
        Mfloor = glm::scale(Mfloor, floorSize);

        R.DrawTexturedCube(Mfloor, floorTex);

        // Pod kupatila
        glm::vec3 floor2Size(8.0f, 0.1f, 11.0f);
        glm::vec3 floor2Pos(0.89f, floorY, 1.98f);

        glm::mat4 Mfloor2 = glm::translate(glm::mat4(1.0f), floor2Pos);
        Mfloor2 = glm::rotate(Mfloor2, glm::radians(180.0f), glm::vec3(0, 0, 1));
        Mfloor2 = glm::scale(Mfloor2, floor2Size);

        R.DrawTexturedCube(Mfloor2, bathroomFloorTex);


        //------------------------------------------------Nameštaj------------------------------------------------

        // WC Solja 
        if (toiletOk)
        {
            glm::vec3 toiletPos = wall2Pos + glm::vec3(0.2f, -0.9f, -0.35f);

            glm::mat4 Mt = glm::translate(glm::mat4(1.0f), toiletPos);
            Mt = glm::rotate(Mt, glm::radians(270.0f), glm::vec3(1,0,0)); 
            Mt = glm::rotate(Mt, glm::radians(180.0f), glm::vec3(0,0,1));
            Mt = glm::scale(Mt, glm::vec3(0.02f));

            R.DrawTexturedMesh(toiletMesh, Mt, toiletTex, glm::vec4(1.0f, 0.99f, 0.96f, 1.0f));

        }

        //Lavabo - Gornji Deo
        if (sinkOk)
        {
            glm::vec3 sinkPos = wall2Pos + glm::vec3(-0.35f, -0.75f, -0.08f);

            glm::mat4 Ms = glm::translate(glm::mat4(1.0f), sinkPos);

            Ms = glm::rotate(Ms, glm::radians(90.0f), glm::vec3(0, 1, 0));
            Ms = glm::scale(Ms, glm::vec3(0.00035f));

            glDisable(GL_CULL_FACE);
            R.DrawMeshTriangles(sinkMesh, Ms, glm::vec4(1.0f, 0.99f, 0.96f, 1.0f), false);
            glEnable(GL_CULL_FACE);
        }

        // Lavabo - Donji Deo
        {
        
            glm::vec3 basePos = wall2Pos + glm::vec3(-0.35f, -1.01f, -0.08f);
            glm::vec3 baseSize = glm::vec3(2.45f, 2.69f, 0.78f);

            glm::mat4 M = glm::translate(glm::mat4(1.0f), basePos);
            M = glm::scale(M, baseSize);

            glm::vec4 bodyCol(0.95f, 0.95f, 0.95f, 1.0f);

            glDisable(GL_CULL_FACE);
            R.DrawTexturedCubeFace(M, cupboardTex, bodyCol, glm::vec4(1,1,1,1), CubeFace::Back, 0.004f);
            glEnable(GL_CULL_FACE);
            
        }

        // Otirač
        if (floorMatOk)
        {
            glm::vec3 matPos = glm::vec3(0.87f, -1.1f, 1.35f);
            glm::mat4 Mm = glm::translate(glm::mat4(1.0f), matPos);

            Mm = glm::rotate(Mm, glm::radians(90.0f), glm::vec3(1,0,0));
            Mm = glm::rotate(Mm, glm::radians(180.0f), glm::vec3(0,1,0));

            Mm = glm::scale(Mm, glm::vec3(0.01f));

            glDisable(GL_CULL_FACE);
            R.DrawMeshTriangles(floorMatMesh, Mm, glm::vec4(0.60f, 0.68f, 0.73f, 1.0f), false);
            glEnable(GL_CULL_FACE);
        }

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
        glm::mat4 Ml = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), gLedPos), glm::radians(90.0f), glm::vec3(1, 0, 0)), glm::vec3(0.085f, 0.08f, 0.08f));

        glm::vec4 ledOff = glm::vec4(0.18f, 0.18f, 0.18f, 1.0f);
        glm::vec4 ledOn  = glm::vec4(0.95f, 0.12f, 0.08f, 1.0f);

        glUniform1f(glGetUniformLocation(shader, "uEmissive"), gAcOn ? 0.8f : 0.0f);
        R.DrawTexturedMesh(R.basin, Ml, lampTex, gAcOn ? ledOn : ledOff);
        glUniform1f(glGetUniformLocation(shader, "uEmissive"), 0.0f);
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

        bool holdingBasin = (gBasinState == BasinState::InFrontFull || gBasinState == BasinState::InFrontEmpty);

        if (!holdingBasin && remoteOk)
        {
            glm::vec3 front = glm::normalize(gCamera.front);
            glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
            glm::vec3 right = glm::normalize(glm::cross(worldUp, front)); 
            glm::vec3 up    = glm::normalize(glm::cross(front, right));  

            float dist = 0.95f;      
            float xOff = 0.05f;      
            float yOff = -0.25f;      

            glm::vec3 pos = gCamera.pos + front * dist + right * xOff + up * yOff;

            glm::mat4 basis(1.0f);
            basis[0] = glm::vec4(right, 0.0f);
            basis[1] = glm::vec4(up, 0.0f);
            basis[2] = glm::vec4(-front, 0.0f);

            glm::mat4 Mr = glm::translate(glm::mat4(1.0f), pos) * basis;
            Mr = glm::rotate(Mr, glm::radians(-18.0f), glm::vec3(1,0,0));
            Mr = glm::rotate(Mr, glm::radians(90.0f), glm::vec3(1,0,0));
            Mr = glm::rotate(Mr, glm::radians(180.0f), glm::vec3(0,0,1));
            Mr = glm::rotate(Mr, glm::radians(-90.0f), glm::vec3(0,1,0));
            Mr = glm::scale(Mr, glm::vec3(0.1f));

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);

            R.DrawMeshTriangles(remoteMat0Mesh, Mr, glm::vec4(1,1,1,1), false);
            R.DrawMeshTriangles(remoteMat1Mesh, Mr, glm::vec4(1,1,1,1), false);
            R.DrawTexturedMesh(remoteMat2Mesh, Mr, remoteTex14, glm::vec4(1,1,1,1));
            R.DrawTexturedMesh(remoteMat3Mesh, Mr, remoteTex8,  glm::vec4(1,1,1,1));
            R.DrawCenter();

            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            R.DrawCenter();
        }


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
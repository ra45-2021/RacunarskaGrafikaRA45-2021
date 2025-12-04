#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <cmath>

#include "../Header/Globals.h"
#include "../Header/Shader.h"
#include "../Header/RenderUtils.h"
#include "../Header/stb_image.h"

Shader* basicShader = nullptr;

float acX = -0.6f;
float acY = 0.60f;
float acW = 1.2f;
float acH = 0.35f;

float lampSize = 0.07f;
float lampX = acX + acW - lampSize - 0.03f;
float lampY = acY + 0.03f; 

float screenW = 0.22f;
float screenH = 0.10f;
float screenY = acY + 0.03f;

float bucketX = -0.4f;
float bucketY = -0.95f;
float bucketW = 0.8f;
float bucketH = 0.25f;


GLFWcursor* loadCursor(const char* path){
    int w,h,ch;
    unsigned char* img = stbi_load(path,&w,&h,&ch,4);
    if(!img) return nullptr;

    GLFWimage image;
    image.width = w;
    image.height = h;
    image.pixels = img;

    GLFWcursor* c = glfwCreateCursor(&image, 0, 0);

    stbi_image_free(img);
    return c;
}


bool pointInQuad(float mx,float my,float x,float y,float w,float h){
    return (mx>=x && mx<=x+w && my>=y && my<=y+h);
}

void mouseToNDC(GLFWwindow* window,double xpos,double ypos,float &nx,float &ny){
    int ww,hh;
    glfwGetWindowSize(window,&ww,&hh);

    nx = (xpos/ww)*2.0f - 1.0f;
    ny = 1.0f - (ypos/hh)*2.0f;
}


void mouse_button_callback(GLFWwindow* window,int button,int action,int mods){
    if(button==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS){

        double xpos,ypos;
        glfwGetCursorPos(window,&xpos,&ypos);

        float mx,my;
        mouseToNDC(window,xpos,ypos,mx,my);

        if(pointInQuad(mx,my,lampX,lampY,lampSize,lampSize)){
            if(!lavorFull)
                klimaOn = !klimaOn;
        }
    }
}

void key_callback(GLFWwindow* w,int key,int sc,int action,int mods){
    if(action==GLFW_PRESS){
        if(key==GLFW_KEY_UP)
            desiredTemp = std::min(40.0f,desiredTemp+1.0f);

        if(key==GLFW_KEY_DOWN)
            desiredTemp = std::max(-10.0f,desiredTemp-1.0f);

        if(key==GLFW_KEY_SPACE){
            water = 0.0f;
            lavorFull = false;
        }

        if(key==GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(w,true);
    }
}

void drawTemperature(float x, float y, float w, float h, int value,
                     unsigned int digitTex[10], unsigned int minusTex)
{
    bool isNegative = value < 0;
    int absValue = std::abs(value);

    int tens = absValue / 10;
    int ones = absValue % 10;

    float spacing = 0.005f;
    float totalWidth = (isNegative ? 3 : 2) * w + (isNegative ? 2 : 1) * spacing;

    float cursorX = x - (totalWidth - (2*w + spacing)) / 2.0f - 0.005f;

    if(isNegative) {
        drawQuad(cursorX, y, w, h, minusTex, 1);
        cursorX += w + spacing;
    }

    drawQuad(cursorX + 0.005f, y, w, h, digitTex[tens], 1);
    cursorX += w + spacing;

    drawQuad(cursorX + 0.005f, y, w, h, digitTex[ones], 1);
}



int main(){

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(mon);
    SCREEN_WIDTH = mode->width;
    SCREEN_HEIGHT = mode->height;

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH,SCREEN_HEIGHT,"Klima",mon,NULL);
    glfwMakeContextCurrent(window);
    glewInit();

    glfwSetMouseButtonCallback(window,mouse_button_callback);
    glfwSetKeyCallback(window,key_callback);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    GLFWcursor* remoteCursor = loadCursor("Resources/cursor.png");
    if(remoteCursor) glfwSetCursor(window,remoteCursor);

    Shader shader("shaders/basic.vs","shaders/basic.fs");
    basicShader = &shader;

    initQuad();
    

    unsigned int fireTex = loadTexture("Resources/fire.png");
    unsigned int snowTex = loadTexture("Resources/snow.png");
    unsigned int okTex   = loadTexture("Resources/check.png");

    unsigned int digitTex[10];
    for(int i = 0; i < 10; i++) {
        std::string path = "Resources/digits/" + std::to_string(i) + ".png";
        digitTex[i] = loadTexture(path.c_str());
    }

    unsigned int minusTex = loadTexture("Resources/minus.png");

    unsigned int nameTex = loadTexture("Resources/name.png");

    unsigned int whiteTex;
    unsigned char px[4]={255,255,255,255};
    glGenTextures(1,&whiteTex);
    glBindTexture(GL_TEXTURE_2D,whiteTex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,px);

    double last = glfwGetTime();
    double target = 1.0/75.0;

    while(!glfwWindowShouldClose(window)){
        double now = glfwGetTime();
        double dt = now-last;
        last = now;

        glClearColor(0.15f,0.15f,0.15f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if(klimaOn){

            vent = std::min<float>(1.0f, vent + dt * 0.5f);

                const float TEMP_SPEED = 1.0f;
                const float TEMP_EPS   = 0.05f;

                float diff = desiredTemp - measuredTemp;

                if (std::fabs(diff) < TEMP_EPS) {
                    measuredTemp = desiredTemp;
                } else if (diff > 0.0f) {
                    measuredTemp += dt * TEMP_SPEED;
                } else {
                    measuredTemp -= dt * TEMP_SPEED;
                }

            if(!lavorFull){
                water += dt * 0.20f;
                if(water >= 1.0f){
                    water = 1.0f;
                    lavorFull = true;
                    klimaOn = false;
                }
            }
        }
        else{
            vent = std::max<float>(0.0f, vent - dt * 0.5f);
        }

        //Klima
        drawQuad(acX, acY, acW, acH, whiteTex, 1, 1,1,1);

        if(klimaOn)
            drawQuad(lampX, lampY, lampSize, lampSize, whiteTex, 1, 1,0,0);
        else
            drawQuad(lampX, lampY, lampSize, lampSize, whiteTex, 1, 0.3f,0.3f,0.3f);

        float ventH = 0.05f * vent;
        drawQuad(acX, acY - ventH, acW, ventH, whiteTex, 1, 0.75f,0.75f,0.75f);


        //Lavor
        drawQuad(bucketX, bucketY, bucketW, bucketH, whiteTex, 0.3f, 0.7f,0.7f,1);
        drawQuad(bucketX, bucketY, bucketW, bucketH * water, whiteTex, 0.7f, 0.4f,0.6f,1);


        float s1X = acX + 0.10f;
        float s2X = s1X + screenW + 0.05f;
        float s3X = s2X + screenW + 0.05f;

        //---------------------------------------------------------
// Screens: black when OFF, light grey when ON
//---------------------------------------------------------

// OFF → all screens black (no numbers)
if (!klimaOn) {
    drawQuad(s1X, screenY, screenW, screenH, whiteTex, 1, 0.0f, 0.0f, 0.0f);
    drawQuad(s2X, screenY, screenW, screenH, whiteTex, 1, 0.0f, 0.0f, 0.0f);
    drawQuad(s3X, screenY, screenW, screenH, whiteTex, 1, 0.0f, 0.0f, 0.0f);
}

// ON → light grey screens + numbers + icon
else {
    drawQuad(s1X, screenY, screenW, screenH, whiteTex, 1, 0.85f, 0.85f, 0.85f);
    drawQuad(s2X, screenY, screenW, screenH, whiteTex, 1, 0.85f, 0.85f, 0.85f);
    drawQuad(s3X, screenY, screenW, screenH, whiteTex, 1, 0.85f, 0.85f, 0.85f);

    float slotW = 0.07f;
    float slotH = 0.07f;

    float centerX1 = s1X + (screenW - (slotW*2 + 0.01f))/2;
    float centerX2 = s2X + (screenW - (slotW*2 + 0.01f))/2;
    float centerY  = screenY + (screenH - slotH)/2 - 0.003f;

    drawTemperature(centerX1, centerY, slotW, slotH, (int)desiredTemp, digitTex, minusTex);
    drawTemperature(centerX2, centerY, slotW, slotH, (int)measuredTemp, digitTex, minusTex);

    // Icon
    float iconSize = 0.10f;
    float iconX = s3X + (screenW - iconSize)/2;
    float iconY = screenY + (screenH - iconSize)/2;

    float iconDiff = desiredTemp - measuredTemp;

    if (std::fabs(iconDiff) < 0.5f)
        drawQuad(iconX,iconY,iconSize,iconSize,okTex,1);
    else if (iconDiff > 0)
        drawQuad(iconX,iconY,iconSize,iconSize,fireTex,1);
    else
        drawQuad(iconX,iconY,iconSize,iconSize,snowTex,1);
}


        //Ime,prezime i index
        float nameW = 0.45f;
        float nameH = 0.22f;
        float nameX = -0.98f;
        float nameY = -0.95f;

        drawQuad(nameX, nameY, nameW, nameH, nameTex, 0.2f, 1,1,1);


        glfwSwapBuffers(window);
        glfwPollEvents();

        double frame = glfwGetTime() - now;
        if(frame < target)
            usleep((target - frame) * 1e6);
    }

    glfwTerminate();
    return 0;
}

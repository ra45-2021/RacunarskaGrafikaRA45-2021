#pragma once
#include <vector>
#include <glm/glm.hpp>

extern bool gUseTex;       
extern bool gTransparent;

struct Droplet {
    glm::vec3 pos;
    glm::vec3 vel;
    bool alive = true;
};

enum class BasinState { OnFloor, InFrontFull, InFrontEmpty };

extern glm::vec3 gAcPos;
extern glm::vec3 gLedPos;
extern float gLedR;

extern bool  gAcOn;
extern float gCoverAngle;

extern glm::vec3   gBasinOriginalPos;
extern glm::vec3   gBasinPos;
extern BasinState  gBasinState;

extern bool  gBasinFull;
extern float gWaterLevel;     

extern std::vector<Droplet> gDrops;

extern double gSpawnAcc;

extern float gBasinRadiusInner;
extern float gBasinHeight;    

extern bool gPrevLmb;
extern bool gPrevSpace;

extern glm::vec3 gLampLocalOffset;
extern float gLampSize;

extern glm::vec3 gNozzleLocalOffset;

extern glm::vec3 gAcScale;
extern glm::vec3 gCoverHingeLocal;
extern glm::vec3 gCoverAfterRotateLocal;
extern glm::vec3 gCoverScale;

extern float gGravity;
extern float gFillPerDrop;
extern float gDropletSize;

extern float desiredTemp;
extern float measuredTemp;

extern bool gPrevLookLed;


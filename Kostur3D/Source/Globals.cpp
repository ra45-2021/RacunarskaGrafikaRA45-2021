#include "Header/Globals.h"

bool gUseTex = false;
bool gTransparent = false;

glm::vec3 gAcPos(-0.2f, 1.0f, -3.0f);
glm::vec3 gLedPos = gAcPos + glm::vec3(1.35f, 0.20f, 0.55f);
float gLedR = 0.04f;

bool  gAcOn = false;
float gCoverAngle = 0.0f;

glm::vec3 gBasinOriginalPos(-0.22f, -1.0f, -3.0f);
glm::vec3 gBasinPos = gBasinOriginalPos;
BasinState gBasinState = BasinState::OnFloor;

bool  gBasinFull = false;
float gWaterLevel = 0.0f;

std::vector<Droplet> gDrops;
double gSpawnAcc = 0.0;

float gBasinRadiusInner = 0.26f;
float gBasinHeight = 0.15f; 

bool gPrevLmb = false;
bool gPrevSpace = false;

glm::vec3 gLampLocalOffset = glm::vec3(1.45f, 0.22f, 0.62f);
float gLampSize = 0.22f;

glm::vec3 gNozzleLocalOffset = glm::vec3(0.0f, -0.25f, 0.55f);

glm::vec3 gAcScale = glm::vec3(3.2f, 1.3f, 1.2f);
glm::vec3 gCoverHingeLocal = glm::vec3(0.0f, 0.25f, 0.60f);
glm::vec3 gCoverAfterRotateLocal = glm::vec3(0.0f, 0.0f, -0.60f);
glm::vec3 gCoverScale = glm::vec3(3.2f, 0.25f, 1.05f);

float gGravity = 0.85f;
float gFillPerDrop = 0.02f;
float gDropletSize = 0.06f;

float desiredTemp = 24.0f;
float measuredTemp = 30.5f;

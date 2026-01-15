#include "Header/Globals.h"

bool gUseTex = false;
bool gTransparent = false;

glm::vec3 gAcPos(-0.2f, 1.0f, -3.0f);
glm::vec3 gLedPos = gAcPos + glm::vec3(1.35f, 0.20f, 0.55f);
float gLedR = 0.10f;

bool  gAcOn = false;
float gCoverAngle = 0.0f;

glm::vec3 gBasinOriginalPos(-0.2f, -1.0f, -2.2f);
glm::vec3 gBasinPos = gBasinOriginalPos;
BasinState gBasinState = BasinState::OnFloor;

bool  gBasinFull = false;
float gWaterLevel = 0.0f;

std::vector<Droplet> gDrops;
double gSpawnAcc = 0.0;

float gBasinRadiusInner = 0.26f;
float gBasinHeight = 0.30f; 

bool gPrevLmb = false;
bool gPrevSpace = false;

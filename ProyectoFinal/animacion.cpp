#include <iostream>
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SOIL2/SOIL2.h"
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void Animation();
void SaveAnimationToFile();
bool LoadAnimationFromFile(const std::string& filename);
void EnsureAnimationsFolder();

const GLuint WIDTH = 800, HEIGHT = 600;
int SCREEN_WIDTH, SCREEN_HEIGHT;

Camera camera(glm::vec3(0.0f, 2.0f, 8.0f));
GLfloat lastX = WIDTH / 2.0f;
GLfloat lastY = HEIGHT / 2.0f;
bool keys[1024];
bool firstMouse = true;

glm::vec3 lightPos(0.0f, 4.0f, 0.0f);
bool active = false;
glm::vec3 Light1 = glm::vec3(0);

glm::vec3 pointLightPositions[] = {
    glm::vec3(0.0f, 4.0f,  0.0f),
    glm::vec3(3.0f, 2.0f,  3.0f),
    glm::vec3(-3.0f, 2.0f, -3.0f),
    glm::vec3(0.0f, 2.0f,  0.0f)
};

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

float vertices[] = {
    -0.5f,-0.5f,-0.5f, 0,0,-1,  0.5f,-0.5f,-0.5f, 0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1,
     0.5f, 0.5f,-0.5f, 0,0,-1, -0.5f, 0.5f,-0.5f, 0,0,-1, -0.5f,-0.5f,-0.5f, 0,0,-1,
    -0.5f,-0.5f, 0.5f, 0,0, 1,  0.5f,-0.5f, 0.5f, 0,0, 1,  0.5f, 0.5f, 0.5f, 0,0, 1,
     0.5f, 0.5f, 0.5f, 0,0, 1, -0.5f, 0.5f, 0.5f, 0,0, 1, -0.5f,-0.5f, 0.5f, 0,0, 1,
    -0.5f, 0.5f, 0.5f,-1,0,0, -0.5f, 0.5f,-0.5f,-1,0,0, -0.5f,-0.5f,-0.5f,-1,0,0,
    -0.5f,-0.5f,-0.5f,-1,0,0, -0.5f,-0.5f, 0.5f,-1,0,0, -0.5f, 0.5f, 0.5f,-1,0,0,
     0.5f, 0.5f, 0.5f, 1,0,0,  0.5f, 0.5f,-0.5f, 1,0,0,  0.5f,-0.5f,-0.5f, 1,0,0,
     0.5f,-0.5f,-0.5f, 1,0,0,  0.5f,-0.5f, 0.5f, 1,0,0,  0.5f, 0.5f, 0.5f, 1,0,0,
    -0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f, 0.5f, 0,-1,0,
     0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f,-0.5f, 0,-1,0,
    -0.5f, 0.5f,-0.5f, 0,1,0,  0.5f, 0.5f,-0.5f, 0,1,0,  0.5f, 0.5f, 0.5f, 0,1,0,
     0.5f, 0.5f, 0.5f, 0,1,0, -0.5f, 0.5f, 0.5f, 0,1,0, -0.5f, 0.5f,-0.5f, 0,1,0
};

typedef struct _frame {
    float posX, posY, posZ;
    float rotY;
    float incX, incY, incZ, incRotY;
    float rTor;
    float rCab;
    float rBD, rAD;
    float rBI, rAI;
    float rPel;
    float rPD, rTD, rPieD;
    float rPI, rTI, rPieI;
    float iTor, iCab;
    float iBD, iAD, iBI, iAI;
    float iPel;
    float iPD, iTD, iPieD;
    float iPI, iTI, iPieI;
} FRAME;

#define MAX_FRAMES 150
#define NUM_CHARS  1

FRAME  KeyFrame[NUM_CHARS][MAX_FRAMES];
int    FrameIndex[NUM_CHARS] = { 0 };
bool   play[NUM_CHARS] = { false };
int    playIndex[NUM_CHARS] = { 0 };
int    i_curr_steps[NUM_CHARS] = { 0 };
int i_max_steps[NUM_CHARS];

float  posX[NUM_CHARS], posY[NUM_CHARS], posZ[NUM_CHARS], rotY_ch[NUM_CHARS];
float  rTor[NUM_CHARS], rCab[NUM_CHARS];
float  rBD[NUM_CHARS], rAD[NUM_CHARS];
float  rBI[NUM_CHARS], rAI[NUM_CHARS];
float  rPel[NUM_CHARS];
float  rPD[NUM_CHARS], rTD[NUM_CHARS], rPieD[NUM_CHARS];
float  rPI[NUM_CHARS], rTI[NUM_CHARS], rPieI[NUM_CHARS];

bool pendingSave = false;
bool pendingLoad = false;

int selectedChar = 0;

struct Waypoint { float x, z, angle; };

Waypoint route[] = {
    {  0.0f,  10.0f,  90.0f },
    {  3.5f,   8.5f, 135.0f },
    {  6.0f,   6.0f, 180.0f },
    {  3.5f,   3.5f, 225.0f },
    {  0.0f,   2.0f, 270.0f },
    { -3.5f,   3.5f, 315.0f },
    { -6.0f,   6.0f, 360.0f },  // <- 360 en vez de 0 para evitar salto
    { -3.5f,   8.5f, 405.0f },  // <- continua sumando en vez de resetear
    {  0.0f,  10.0f, 450.0f }   // <- 450 = 90 + 360, mismo angulo final
};
const int ROUTE_SIZE = 9;
void EnsureAnimationsFolder() {
#if defined(_WIN32)
    _mkdir("animations");
#else
    mkdir("animations", 0755);
#endif
}

void BuildWalkCycle(int c, float phaseOffset) {
    const float WALK_Y = 1.15f;

    float wx = route[0].x, wz = route[0].z;
    posX[c] = wx + phaseOffset * 0.3f;
    posY[c] = WALK_Y;
    posZ[c] = wz;
    rotY_ch[c] = route[0].angle;
    rTor[c] = 0; rCab[c] = 5; rPel[c] = 0;
    rBD[c] = 0;  rAD[c] = 0;  rBI[c] = 0; rAI[c] = 0;
    rPD[c] = 0;  rTD[c] = 0;  rPieD[c] = 0;
    rPI[c] = 0;  rTI[c] = 0;  rPieI[c] = 0;

    FrameIndex[c] = 0;

#define SAVE_KF(ci) do { \
        KeyFrame[ci][FrameIndex[ci]].posX  = posX[ci]; \
        KeyFrame[ci][FrameIndex[ci]].posY  = posY[ci]; \
        KeyFrame[ci][FrameIndex[ci]].posZ  = posZ[ci]; \
        KeyFrame[ci][FrameIndex[ci]].rotY  = rotY_ch[ci]; \
        KeyFrame[ci][FrameIndex[ci]].rTor  = rTor[ci]; \
        KeyFrame[ci][FrameIndex[ci]].rCab  = rCab[ci]; \
        KeyFrame[ci][FrameIndex[ci]].rPel  = rPel[ci]; \
        KeyFrame[ci][FrameIndex[ci]].rBD   = rBD[ci];  \
        KeyFrame[ci][FrameIndex[ci]].rAD   = rAD[ci];  \
        KeyFrame[ci][FrameIndex[ci]].rBI   = rBI[ci];  \
        KeyFrame[ci][FrameIndex[ci]].rAI   = rAI[ci];  \
        KeyFrame[ci][FrameIndex[ci]].rPD   = rPD[ci];  \
        KeyFrame[ci][FrameIndex[ci]].rTD   = rTD[ci];  \
        KeyFrame[ci][FrameIndex[ci]].rPieD = rPieD[ci]; \
        KeyFrame[ci][FrameIndex[ci]].rPI   = rPI[ci];  \
        KeyFrame[ci][FrameIndex[ci]].rTI   = rTI[ci];  \
        KeyFrame[ci][FrameIndex[ci]].rPieI = rPieI[ci]; \
        FrameIndex[ci]++; \
    } while(0)

    SAVE_KF(c);

    for (int seg = 0; seg < ROUTE_SIZE - 1; seg++) {
        float ax = route[seg].x, az = route[seg].z, aa = route[seg].angle;
        float bx = route[seg + 1].x, bz = route[seg + 1].z, ba = route[seg + 1].angle;

        // Cuanto gira en este segmento (camino corto)
        float deltaAngle = ba - aa;
        while (deltaAngle > 180.0f) deltaAngle -= 360.0f;
        while (deltaAngle < -180.0f) deltaAngle += 360.0f;
        float leanTor = deltaAngle * 0.08f;

        for (int sub = 1; sub <= 4; sub++) {
            float t = sub / 4.0f;
            float mx = ax + (bx - ax) * t;
            float mz = az + (bz - az) * t;
            float ma = aa + (ba - aa) * t;

            bool rightLead = ((seg * 4 + sub + (int)(phaseOffset * 2)) % 2 == 0);

            posX[c] = mx; posY[c] = WALK_Y; posZ[c] = mz; rotY_ch[c] = ma;
            rTor[c] = 3.0f;
            rCab[c] = 5.0f;
            rPel[c] = 2.0f + leanTor * 0.3f;

            if (rightLead) {
                rPD[c] = -25.0f;  rTD[c] = 10.0f;  rPieD[c] = -5.0f;
                rPI[c] = 20.0f;  rTI[c] = 5.0f;  rPieI[c] = 5.0f;
                rBI[c] = 12.0f + leanTor * 0.5f;  rAI[c] = 0.0f;
                rBD[c] = -12.0f - leanTor * 0.5f;  rAD[c] = 0.0f;
            }
            else {
                rPI[c] = -25.0f;  rTI[c] = 10.0f;  rPieI[c] = -5.0f;
                rPD[c] = 20.0f;  rTD[c] = 5.0f;  rPieD[c] = 5.0f;
                rBD[c] = 12.0f + leanTor * 0.5f;  rAD[c] = 0.0f;
                rBI[c] = -12.0f - leanTor * 0.5f;  rAI[c] = 0.0f;
            }

            SAVE_KF(c);
        }
    }

#undef SAVE_KF

    // Normaliza todos los rotY al rango (-180, 180]
    for (int i = 0; i < FrameIndex[c]; i++) {
        while (KeyFrame[c][i].rotY > 180.0f) KeyFrame[c][i].rotY -= 360.0f;
        while (KeyFrame[c][i].rotY < -180.0f) KeyFrame[c][i].rotY += 360.0f;
    }

    printf("[Char %d] %d keyframes generados (fase=%.1f)\n", c, FrameIndex[c], phaseOffset);
}

void interpolation(int c) {
    int p = playIndex[c];

    // Calcula pasos proporcionales a la distancia del segmento
    float dx = KeyFrame[c][p + 1].posX - KeyFrame[c][p].posX;
    float dz = KeyFrame[c][p + 1].posZ - KeyFrame[c][p].posZ;
    float dist = sqrtf(dx * dx + dz * dz);
    int steps = (int)(dist * 400.0f);
    if (steps < 200) steps = 200;
    i_max_steps[c] = steps;

#define LERP(field, inc_field) \
        KeyFrame[c][p].inc_field = (KeyFrame[c][p+1].field - KeyFrame[c][p].field) / steps

    LERP(posX, incX);
    LERP(posY, incY);
    LERP(posZ, incZ);

    float fromA = KeyFrame[c][p].rotY;
    float toA = KeyFrame[c][p + 1].rotY;
    float diff = toA - fromA;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    KeyFrame[c][p].incRotY = diff / steps;

    LERP(rTor, iTor);
    LERP(rCab, iCab);
    LERP(rPel, iPel);
    LERP(rBD, iBD);
    LERP(rAD, iAD);
    LERP(rBI, iBI);
    LERP(rAI, iAI);
    LERP(rPD, iPD);
    LERP(rTD, iTD);
    LERP(rPieD, iPieD);
    LERP(rPI, iPI);
    LERP(rTI, iTI);
    LERP(rPieI, iPieI);
#undef LERP
}


void resetChar(int c) {
    posX[c] = KeyFrame[c][0].posX;
    posY[c] = KeyFrame[c][0].posY;
    posZ[c] = KeyFrame[c][0].posZ;
    rotY_ch[c] = KeyFrame[c][0].rotY;
    rTor[c] = KeyFrame[c][0].rTor;
    rCab[c] = KeyFrame[c][0].rCab;
    rPel[c] = KeyFrame[c][0].rPel;
    rBD[c] = KeyFrame[c][0].rBD;
    rAD[c] = KeyFrame[c][0].rAD;
    rBI[c] = KeyFrame[c][0].rBI;
    rAI[c] = KeyFrame[c][0].rAI;
    rPD[c] = KeyFrame[c][0].rPD;
    rTD[c] = KeyFrame[c][0].rTD;
    rPieD[c] = KeyFrame[c][0].rPieD;
    rPI[c] = KeyFrame[c][0].rPI;
    rTI[c] = KeyFrame[c][0].rTI;
    rPieI[c] = KeyFrame[c][0].rPieI;
}

void Animation() {
    for (int c = 0; c < NUM_CHARS; c++) {
        if (!play[c]) continue;

        if (i_curr_steps[c] >= i_max_steps[c]) {
            playIndex[c]++;
            if (playIndex[c] > FrameIndex[c] - 2) {
                playIndex[c] = 0;
                i_curr_steps[c] = 0;
                resetChar(c);
                interpolation(c);
            }
            else {
                i_curr_steps[c] = 0;
                interpolation(c);
            }
        }
        else {
            int p = playIndex[c];
            posX[c] += KeyFrame[c][p].incX;
            posY[c] += KeyFrame[c][p].incY;
            posZ[c] += KeyFrame[c][p].incZ;
            rotY_ch[c] += KeyFrame[c][p].incRotY;
            while (rotY_ch[c] > 180.0f) rotY_ch[c] -= 360.0f;
            while (rotY_ch[c] < -180.0f) rotY_ch[c] += 360.0f;
            rTor[c] += KeyFrame[c][p].iTor;
            rCab[c] += KeyFrame[c][p].iCab;
            rPel[c] += KeyFrame[c][p].iPel;
            rBD[c] += KeyFrame[c][p].iBD;
            rAD[c] += KeyFrame[c][p].iAD;
            rBI[c] += KeyFrame[c][p].iBI;
            rAI[c] += KeyFrame[c][p].iAI;
            rPD[c] += KeyFrame[c][p].iPD;
            rTD[c] += KeyFrame[c][p].iTD;
            rPieD[c] += KeyFrame[c][p].iPieD;
            rPI[c] += KeyFrame[c][p].iPI;
            rTI[c] += KeyFrame[c][p].iTI;
            rPieI[c] += KeyFrame[c][p].iPieI;
            i_curr_steps[c]++;
        }
    }
}
// ---------------------------------------------------------------------------
// DrawCharacter
// TB_D = elder_Female_tbrazo_d.obj  (brazo completo derecho, T-pose en X)
// TB_I = elder_Female_tbrazo_i.obj  (brazo completo izquierdo, T-pose en X)
//
// tbrazo_d: hombro en X=-0.52, mano en X=+0.66, origen en centro geometrico
// tbrazo_i: hombro en X=+0.50, mano en X=-0.67, origen en centro geometrico
//
// Cadena de transformaciones para cada brazo:
//   1. Ir al hombro del torso en espacio del torso
//   2. Compensar el pivot: mover el hombro del modelo al origen
//   3. Rotar -90Z (derecho) / +90Z (izquierdo) para bajar el brazo
//   4. Rotar rBD/rBI en X para el balanceo de caminata
// ---------------------------------------------------------------------------
void DrawCharacter(int c, Shader& shader, GLint mLoc,
    Model& Torso, Model& Pelvis, Model& Cabeza,
    Model& TB_D, Model& TB_I,
    Model& P_D, Model& T_D, Model& Pie_D,
    Model& P_I, Model& T_I, Model& Pie_I)
{
    // Raiz: posicion mundial + orientacion Y
    glm::mat4 mG = glm::translate(glm::mat4(1.0f),
        glm::vec3(posX[c], posY[c], posZ[c]));
    mG = glm::rotate(mG, glm::radians(rotY_ch[c]), glm::vec3(0, 1, 0));
    mG = glm::scale(mG, glm::vec3(0.65f));

    // --- Torso ---
    glm::mat4 mT = glm::rotate(mG, glm::radians(rTor[c]), glm::vec3(1, 0, 0));
    mT = glm::translate(mT, glm::vec3(0.0f, 0.2f, 0.0f));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mT));
    Torso.Draw(shader);

    // --- Cabeza ---
    glm::mat4 mC = glm::translate(mT, glm::vec3(0.0f, 0.44f, 0.0f));
    mC = glm::rotate(mC, glm::radians(rCab[c]), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mC));
    Cabeza.Draw(shader);

    // ================= BRAZO DERECHO (tbrazo_d) =================
    // Hombro del torso en X=-0.28, Y=+0.20
    // Pivot del modelo (hombro) en X=-0.52 -> compensamos +0.52 en X
    // Rotate -90Z convierte eje +X en -Y (brazo cuelga hacia abajo)
    // Rotate rBD en X: balanceo adelante/atras al caminar
    glm::mat4 mBD = glm::translate(mT, glm::vec3(-0.10f, -0.55f, -0.08f));
    mBD = glm::rotate(mBD, glm::radians(42.0f), glm::vec3(0, 1, 1));
    mBD = glm::translate(mBD, glm::vec3(0.0f, 0.0f, 0.2f));
    mBD = glm::translate(mBD, glm::vec3(0.50f, 0.0f, 0.0f));
    mBD = glm::rotate(mBD, glm::radians(-90.0f), glm::vec3(0, 0, 1));
    mBD = glm::rotate(mBD, glm::radians(rBD[c]), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mBD));
    TB_D.Draw(shader);

    // ================= BRAZO IZQUIERDO (tbrazo_i) =================
    // Hombro del torso en X=+0.28, Y=+0.20
    // Pivot del modelo (hombro) en X=+0.50 -> compensamos -0.50 en X
    // Rotate +90Z convierte eje -X en -Y (brazo cuelga hacia abajo)
    // Rotate rBI en X: balanceo adelante/atras al caminar
    glm::mat4 mBI = glm::translate(mT, glm::vec3(0.10f, -0.50f, 0.1f));
    mBI = glm::rotate(mBI, glm::radians(-42.0f), glm::vec3(0, 1, 1));
   // mBD = glm::translate(mBD, glm::vec3(0.0f, 0.0f, 0.2f));
    mBI = glm::translate(mBI, glm::vec3(-0.50f, 0.0f, 0.0f));
    mBI = glm::rotate(mBI, glm::radians(90.0f), glm::vec3(0, 0, 1));
    mBI = glm::rotate(mBI, glm::radians(rBI[c]), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mBI));
    TB_I.Draw(shader);


    // --- Pelvis ---
    glm::mat4 mPelBase = glm::translate(mG, glm::vec3(0.0f, -0.28f, 0.0f));
    glm::mat4 mPel = glm::rotate(mPelBase, glm::radians(rPel[c]), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPel));
    Pelvis.Draw(shader);

    // --- Pierna Derecha ---
    glm::mat4 mPD = glm::translate(mPel, glm::vec3(-0.15f, -0.46f, 0.0f));
    mPD = glm::rotate(mPD, glm::radians(rPD[c]), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPD));
    P_D.Draw(shader);

    // --- Tobillo Derecho ---
    glm::mat4 mTD = glm::translate(mPD, glm::vec3(0.0f, -0.27f, 0.0f));
    mTD = glm::rotate(mTD, glm::radians(rTD[c]), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mTD));
    T_D.Draw(shader);

    // --- Pie Derecho ---
    glm::mat4 mPieD = glm::translate(mTD, glm::vec3(0.0f, -0.49f, 0.05f));
    mPieD = glm::rotate(mPieD, glm::radians(rPieD[c]), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPieD));
    Pie_D.Draw(shader);

    // --- Pierna Izquierda ---
    glm::mat4 mPI = glm::translate(mPel, glm::vec3(0.15f, -0.46f, 0.0f));
    mPI = glm::rotate(mPI, glm::radians(rPI[c]), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPI));
    P_I.Draw(shader);

    // --- Tobillo Izquierdo ---
    glm::mat4 mTI = glm::translate(mPI, glm::vec3(0.0f, -0.27f, 0.0f));
    mTI = glm::rotate(mTI, glm::radians(rTI[c]), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mTI));
    T_I.Draw(shader);

    // --- Pie Izquierdo ---
    glm::mat4 mPieI = glm::translate(mTI, glm::vec3(0.0f, -0.49f, 0.05f));
    mPieI = glm::rotate(mPieI, glm::radians(rPieI[c]), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPieI));
    Pie_I.Draw(shader);
}

void SaveAnimationToFile() {
    if (FrameIndex[0] < 2) { printf("Necesitas al menos 2 keyframes.\n"); return; }
    EnsureAnimationsFolder();
    std::string name;
    printf("Nombre del archivo (sin extension): ");
    fflush(stdout);
    std::cin >> name;
    std::string path = "animations/" + name + ".txt";
    std::ofstream file(path);
    if (!file.is_open()) { printf("No se pudo crear el archivo.\n"); return; }
    file << "FRAMES " << FrameIndex[0] << "\n";
    for (int i = 0; i < FrameIndex[0]; i++) {
        file << "FRAME " << i << "\n";
        file << "posX " << KeyFrame[0][i].posX << "\n";
        file << "posY " << KeyFrame[0][i].posY << "\n";
        file << "posZ " << KeyFrame[0][i].posZ << "\n";
        file << "rotY " << KeyFrame[0][i].rotY << "\n";
        file << "rTor " << KeyFrame[0][i].rTor << "\n";
        file << "rCab " << KeyFrame[0][i].rCab << "\n";
        file << "rPel " << KeyFrame[0][i].rPel << "\n";
        file << "rBD " << KeyFrame[0][i].rBD << "\n";
        file << "rAD " << KeyFrame[0][i].rAD << "\n";
        file << "rBI " << KeyFrame[0][i].rBI << "\n";
        file << "rAI " << KeyFrame[0][i].rAI << "\n";
        file << "rPD " << KeyFrame[0][i].rPD << "\n";
        file << "rTD " << KeyFrame[0][i].rTD << "\n";
        file << "rPieD " << KeyFrame[0][i].rPieD << "\n";
        file << "rPI " << KeyFrame[0][i].rPI << "\n";
        file << "rTI " << KeyFrame[0][i].rTI << "\n";
        file << "rPieI " << KeyFrame[0][i].rPieI << "\n";
        file << "END_FRAME\n";
    }
    file.close();
    printf("Guardado: %s (%d frames)\n", path.c_str(), FrameIndex[0]);
}

bool LoadAnimationFromFile(const std::string& filename) {
    std::string path = "animations/" + filename + ".txt";
    std::ifstream file(path);
    if (!file.is_open()) { printf("No se pudo abrir: %s\n", path.c_str()); return false; }
    for (int i = 0; i < MAX_FRAMES; i++) memset(&KeyFrame[0][i], 0, sizeof(FRAME));
    std::string line;
    int cur = -1;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string key; ss >> key;
        if (key == "FRAMES") continue;
        if (key == "FRAME") { cur++; continue; }
        if (key == "END_FRAME") continue;
        float val; if (!(ss >> val)) continue;
        if (cur < 0 || cur >= MAX_FRAMES) continue;
        if (key == "posX")  KeyFrame[0][cur].posX = val;
        else if (key == "posY")  KeyFrame[0][cur].posY = val;
        else if (key == "posZ")  KeyFrame[0][cur].posZ = val;
        else if (key == "rotY")  KeyFrame[0][cur].rotY = val;
        else if (key == "rTor")  KeyFrame[0][cur].rTor = val;
        else if (key == "rCab")  KeyFrame[0][cur].rCab = val;
        else if (key == "rPel")  KeyFrame[0][cur].rPel = val;
        else if (key == "rBD")   KeyFrame[0][cur].rBD = val;
        else if (key == "rAD")   KeyFrame[0][cur].rAD = val;
        else if (key == "rBI")   KeyFrame[0][cur].rBI = val;
        else if (key == "rAI")   KeyFrame[0][cur].rAI = val;
        else if (key == "rPD")   KeyFrame[0][cur].rPD = val;
        else if (key == "rTD")   KeyFrame[0][cur].rTD = val;
        else if (key == "rPieD") KeyFrame[0][cur].rPieD = val;
        else if (key == "rPI")   KeyFrame[0][cur].rPI = val;
        else if (key == "rTI")   KeyFrame[0][cur].rTI = val;
        else if (key == "rPieI") KeyFrame[0][cur].rPieI = val;
    }
    file.close();
    FrameIndex[0] = cur + 1;
    printf("%d frames cargados para personaje 0.\n", FrameIndex[0]);
    return FrameIndex[0] > 0;
}

void DoMovement() {
    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])    camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])  camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])  camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, deltaTime);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) { selectedChar = 0; printf("Personaje 0 seleccionado\n"); }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) { selectedChar = 1; printf("Personaje 1 seleccionado\n"); }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) { selectedChar = 2; printf("Personaje 2 seleccionado\n"); }

    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        for (int c = 0; c < NUM_CHARS; c++) {
            if (FrameIndex[c] > 1) {
                resetChar(c);
                playIndex[c] = 0;
                i_curr_steps[c] = 0;
                interpolation(c);
                play[c] = true;
            }
        }
        printf("Animacion iniciada para %d personajes\n", NUM_CHARS);
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        for (int c = 0; c < NUM_CHARS; c++) play[c] = false;
        printf("Animacion detenida (tecla M)\n");
    }

    if (key == GLFW_KEY_K && action == GLFW_PRESS) {
        int c = selectedChar;
        if (FrameIndex[c] < MAX_FRAMES) {
            KeyFrame[c][FrameIndex[c]].posX = posX[c];
            KeyFrame[c][FrameIndex[c]].posY = posY[c];
            KeyFrame[c][FrameIndex[c]].posZ = posZ[c];
            KeyFrame[c][FrameIndex[c]].rotY = rotY_ch[c];
            KeyFrame[c][FrameIndex[c]].rTor = rTor[c];
            KeyFrame[c][FrameIndex[c]].rCab = rCab[c];
            KeyFrame[c][FrameIndex[c]].rPel = rPel[c];
            KeyFrame[c][FrameIndex[c]].rBD = rBD[c];
            KeyFrame[c][FrameIndex[c]].rAD = rAD[c];
            KeyFrame[c][FrameIndex[c]].rBI = rBI[c];
            KeyFrame[c][FrameIndex[c]].rAI = rAI[c];
            KeyFrame[c][FrameIndex[c]].rPD = rPD[c];
            KeyFrame[c][FrameIndex[c]].rTD = rTD[c];
            KeyFrame[c][FrameIndex[c]].rPieD = rPieD[c];
            KeyFrame[c][FrameIndex[c]].rPI = rPI[c];
            KeyFrame[c][FrameIndex[c]].rTI = rTI[c];
            KeyFrame[c][FrameIndex[c]].rPieI = rPieI[c];
            FrameIndex[c]++;
            printf("KF guardado para personaje %d (total %d)\n", c, FrameIndex[c]);
        }
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) pendingSave = true;
    if (key == GLFW_KEY_O && action == GLFW_PRESS) pendingLoad = true;

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        active = !active;
        Light1 = active ? glm::vec3(0.2f, 0.8f, 1.0f) : glm::vec3(0);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)   keys[key] = true;
        if (action == GLFW_RELEASE) keys[key] = false;
    }
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
    if (firstMouse) { lastX = (float)xPos; lastY = (float)yPos; firstMouse = false; }
    camera.ProcessMouseMovement((float)(xPos - lastX), (float)(lastY - yPos));
    lastX = (float)xPos;
    lastY = (float)yPos;
}

int main() {
    EnsureAnimationsFolder();

    {
        char resp;
        printf("Cargar animacion desde archivo? (s/n): ");
        fflush(stdout);
        std::cin >> resp;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (resp == 's' || resp == 'S') {
            std::string name;
            printf("Nombre (sin extension): ");
            fflush(stdout);
            std::cin >> name;
            LoadAnimationFromFile(name);
        }
    }

    for (int c = 0; c < NUM_CHARS; c++)
        for (int i = 0; i < MAX_FRAMES; i++)
            memset(&KeyFrame[c][i], 0, sizeof(FRAME));

    
    for (int c = 0; c < NUM_CHARS; c++) i_max_steps[c] = 1500;

    BuildWalkCycle(0, 0.0f);



    for (int c = 0; c < NUM_CHARS; c++) {
        if (FrameIndex[c] > 1) {
            resetChar(c);
            playIndex[c] = 0;
            i_curr_steps[c] = 0;
            interpolation(c);
            play[c] = true;
        }
    }

    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Caminata elder_Female", nullptr, nullptr);
    if (!window) { glfwTerminate(); return EXIT_FAILURE; }
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return EXIT_FAILURE;
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
    Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

    // --- Modelos ---
    Model Torso((char*)"Models/mujer1/elder_Female_torso.obj");
    Model Pelvis((char*)"Models/mujer1/elder_Female_pelvis.obj");
    Model Cabeza((char*)"Models/mujer1/elder_Female_cabeza.obj");
    Model TB_D((char*)"Models/mujer1/elder_Female_tbrazo_d.obj");
    Model TB_I((char*)"Models/mujer1/elder_Female_tbrazo_i.obj");
    Model P_D((char*)"Models/mujer1/elder_Female_pierna_d.obj");
    Model T_D((char*)"Models/mujer1/elder_Female_tobillo_d.obj");
    Model Pie_D((char*)"Models/mujer1/elder_Female_pie_d.obj");
    Model P_I((char*)"Models/mujer1/elder_Female_pierna_i.obj");
    Model T_I((char*)"Models/mujer1/elder_Female_tobillo_i.obj");
    Model Pie_I((char*)"Models/mujer1/elder_Female_pie_i.obj");
    Model Piso((char*)"Models/lobby/lobby.obj");

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    lightingShader.Use();
    glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.difuse"), 0);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.specular"), 1);

    glm::mat4 projection = glm::perspective(camera.GetZoom(),
        (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

    while (!glfwWindowShouldClose(window)) {
        GLfloat currentFrame = (GLfloat)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        DoMovement();
        Animation();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (pendingSave) { pendingSave = false; SaveAnimationToFile(); }
        if (pendingLoad) {
            pendingLoad = false;
            std::string name;
            printf("Archivo a cargar: "); fflush(stdout); std::cin >> name;
            if (LoadAnimationFromFile(name)) {
                play[0] = false; playIndex[0] = 0; i_curr_steps[0] = 0; resetChar(0);
            }
        }

        lightingShader.Use();
        glUniform1i(glGetUniformLocation(lightingShader.Program, "diffuse"), 0);

        GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
        glUniform3f(viewPosLoc,
            camera.GetPosition().x,
            camera.GetPosition().y,
            camera.GetPosition().z);

        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"), 0.6f, 0.6f, 0.6f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), 0.6f, 0.6f, 0.6f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"), 0.3f, 0.3f, 0.3f);

        glm::vec3 lc;
        lc.x = fabs(sinf(glfwGetTime() * Light1.x));
        lc.y = fabs(sinf(glfwGetTime() * Light1.y));
        lc.z = sinf(glfwGetTime() * Light1.z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].position"),
            pointLightPositions[0].x, pointLightPositions[0].y, pointLightPositions[0].z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].ambient"), lc.x, lc.y, lc.z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].diffuse"), lc.x, lc.y, lc.z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].specular"), 1.0f, 0.2f, 0.2f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].linear"), 0.045f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].quadratic"), 0.075f);

        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.position"),
            camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.direction"),
            camera.GetFront().x, camera.GetFront().y, camera.GetFront().z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), 0.2f, 0.2f, 0.8f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), 0.2f, 0.2f, 0.8f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.specular"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"), 0.3f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.7f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"),
            glm::cos(glm::radians(12.0f)));
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"),
            glm::cos(glm::radians(18.0f)));

        glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 5.0f);

        glm::mat4 view = camera.GetViewMatrix();
        GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
        GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
        GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Piso
        glm::mat4 mPiso = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mPiso));
        Piso.Draw(lightingShader);

        
        // Dibujar personajes
        for (int c = 0; c < NUM_CHARS; c++) {
            DrawCharacter(c, lightingShader, modelLoc,
                Torso, Pelvis, Cabeza,
                TB_D, TB_I,
                P_D, T_D, Pie_D,
                P_I, T_I, Pie_I);
        }

        // Lampara indicadora
        lampShader.Use();
        GLint lModelLoc = glGetUniformLocation(lampShader.Program, "model");
        GLint lViewLoc = glGetUniformLocation(lampShader.Program, "view");
        GLint lProjLoc = glGetUniformLocation(lampShader.Program, "projection");
        glUniformMatrix4fv(lViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(lProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glm::mat4 mLamp = glm::translate(glm::mat4(1.0f), pointLightPositions[0]);
        mLamp = glm::scale(mLamp, glm::vec3(0.2f));
        glUniformMatrix4fv(lModelLoc, 1, GL_FALSE, glm::value_ptr(mLamp));
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
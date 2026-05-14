// =============================================================================
// PROYECTO FINAL CGIHC - INTEGRADO
//   - Escena: lobby (trolleo.obj) + stands + lamparas + skybox
//   - Animacion 1 (grua torre + brazo robotico): se carga desde
//       Animations/animacion_grua.txt
//       Animations/animacion_pick_place.txt
//   - Animacion 2 (personaje elder_Female caminando): ciclo de caminata
//       generado proceduralmente al inicio (BuildWalkCycle).
//
//   Controles principales:
//     WASD / flechas : mover camara
//     raton          : mirar
//     B              : encender/apagar lamparas
//     G              : encender/apagar animacion de GRUA + BRAZO
//     N              : encender/apagar animacion de PERSONAJE
//     ESC            : salir
// =============================================================================

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

// GLEW / GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Other libs
#include "SOIL2/SOIL2.h"
#include "stb_image.h"

// GLM
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

// Otros
#include "Camera.h"
#include "Model.h"
#include "Shader.h"
#include "Texture.h"

using namespace std;

#if defined(_WIN32)
#define dir "Animations\\"
#else
#define dir "Animations/"
#endif

// =============================================================================
// PROTOTIPOS
// =============================================================================
void KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                 int mode);
void MouseCallback(GLFWwindow *window, double xPos, double yPos);
void DoMovement();

// Animacion de la grua + brazo robotico (sistema "v[20]")
void Animation();
void resetElements();
void loadFromFile();
void applyFrame(int index);

// Animacion del personaje (sistema "FRAME por campos")
void CharAnimation();
void interpolation(int c);
void resetChar(int c);
void BuildWalkCycle(int c, float phaseOffset);

// =============================================================================
// VENTANA Y CAMARA
// =============================================================================
const GLuint WIDTH = 1200, HEIGHT = 800;
int SCREEN_WIDTH, SCREEN_HEIGHT;

Camera camera(glm::vec3(0.0f, 5.0f, 20.0f));
GLfloat lastX = WIDTH / 2.0f;
GLfloat lastY = HEIGHT / 2.0f;
bool keys[1024];
bool firstMouse = true;
bool lampsOn = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// =============================================================================
// LAMPARAS (posiciones en mundo)
// =============================================================================
glm::vec3 posLamp1[6] = {
    {0.0000f, 3.66f, -3.4804f}, {6.4402f, 3.66f, -3.4804f},
    {6.4402f, 3.66f, 2.7842f},  {0.0000f, 3.66f, 2.7842f},
    {-7.3793f, 3.66f, 4.9873f}, {-7.3793f, 3.66f, 1.0176f},
};
glm::vec3 posLamp2[5] = {
    {1.1049f, 3.66f, 11.2860f},  {-1.5901f, 3.66f, 11.2860f},
    {-7.3817f, 3.66f, 8.7838f},  {-7.3817f, 3.66f, -3.5014f},
    {-7.3817f, 3.66f, -8.6545f},
};

// =============================================================================
// SISTEMA DE ANIMACION 1: GRUA TORRE + BRAZO ROBOTICO
// =============================================================================
float gruaPosX = 0.0f, gruaPosY = 0.0f, gruaPosZ = 0.0f;
float rotTorreta = 0.0f, elevacionPluma = 25.0f, extensionTelescopio = 0.0f,
      largoCableActual = 6.0f;
float anguloPenduloX = 0.0f, anguloPenduloZ = 0.0f;

float baseY = 0.0f, hombroX = 0.0f, hombroZ = 0.0f, codo = 0.0f, munecaX = 0.0f,
      munecaZ = 0.0f;
float flexPulgar = 0.0f, flexIndice = 0.0f, flexMedio = 0.0f, flexAnular = 0.0f,
      flexMenique = 0.0f;

#define ANIM_VARS 20
#define MAX_FRAMES 150
int i_max_steps = 150;
int i_curr_steps = 0;

typedef struct _frame {
  float v[ANIM_VARS];
} FRAME;

FRAME KeyFrame[MAX_FRAMES];
int FrameIndex = 0;
bool play = false;
int playIndex = 0;

void applyFrame(int index) {
  gruaPosX = KeyFrame[index].v[0];
  gruaPosY = KeyFrame[index].v[1];
  gruaPosZ = KeyFrame[index].v[2];
  rotTorreta = KeyFrame[index].v[3];
  elevacionPluma = KeyFrame[index].v[4];
  extensionTelescopio = KeyFrame[index].v[5];
  largoCableActual = KeyFrame[index].v[6];
  anguloPenduloX = KeyFrame[index].v[7];
  anguloPenduloZ = KeyFrame[index].v[8];

  baseY = KeyFrame[index].v[9];
  hombroX = KeyFrame[index].v[10];
  hombroZ = KeyFrame[index].v[11];
  codo = KeyFrame[index].v[12];
  munecaX = KeyFrame[index].v[13];
  munecaZ = KeyFrame[index].v[14];
  flexPulgar = KeyFrame[index].v[15];
  flexIndice = KeyFrame[index].v[16];
  flexMedio = KeyFrame[index].v[17];
  flexAnular = KeyFrame[index].v[18];
  flexMenique = KeyFrame[index].v[19];
}

void resetElements(void) {
  if (FrameIndex > 0)
    applyFrame(0);
}

void loadFromFile() {
  ifstream fGrua(dir + string("animacion_grua.txt"));
  ifstream fBrazo(dir + string("animacion_pick_place.txt"));

  int framesGrua = 0;
  int framesBrazo = 0;

  if (fGrua.is_open())
    fGrua >> framesGrua;
  if (fBrazo.is_open())
    fBrazo >> framesBrazo;

  FrameIndex = max(framesGrua, framesBrazo);
  if (FrameIndex > MAX_FRAMES)
    FrameIndex = MAX_FRAMES;

  for (int i = 0; i < FrameIndex; i++) {
    for (int j = 0; j < ANIM_VARS; j++)
      KeyFrame[i].v[j] = 0.0f;
    KeyFrame[i].v[4] = 25.0f; // elevacionPluma por defecto
    KeyFrame[i].v[6] = 6.0f;  // largoCable por defecto

    if (i < framesGrua && fGrua.is_open()) {
      for (int j = 0; j < 9; j++)
        fGrua >> KeyFrame[i].v[j];
    } else if (i > 0) {
      for (int j = 0; j < 9; j++)
        KeyFrame[i].v[j] = KeyFrame[i - 1].v[j];
    }

    if (i < framesBrazo && fBrazo.is_open()) {
      for (int j = 9; j < 20; j++)
        fBrazo >> KeyFrame[i].v[j];
    } else if (i > 0) {
      for (int j = 9; j < 20; j++)
        KeyFrame[i].v[j] = KeyFrame[i - 1].v[j];
    }
  }

  if (fGrua.is_open())
    fGrua.close();
  if (fBrazo.is_open())
    fBrazo.close();

  if (FrameIndex > 1) {
    cout << "[Grua/Brazo] Animaciones combinadas cargadas. Frames: "
         << FrameIndex << endl;
    play = true;
    resetElements();
  } else {
    cout << "[Grua/Brazo] ADVERTENCIA: no se encontraron los archivos .txt."
         << endl;
  }
}

void Animation() {
  if (!play || FrameIndex < 2)
    return;

  if (i_curr_steps >= i_max_steps) {
    playIndex++;
    if (playIndex > FrameIndex - 2) {
      playIndex = 0;
      resetElements();
    }
    i_curr_steps = 0;
  } else {
    float t = (float)i_curr_steps / (float)i_max_steps;
    float smooth_t = t * t * (3.0f - 2.0f * t);

    auto lerp = [&](int varIdx) {
      return KeyFrame[playIndex].v[varIdx] +
             (KeyFrame[playIndex + 1].v[varIdx] -
              KeyFrame[playIndex].v[varIdx]) *
                 smooth_t;
    };

    gruaPosX = lerp(0);
    gruaPosY = lerp(1);
    gruaPosZ = lerp(2);
    rotTorreta = lerp(3);
    elevacionPluma = lerp(4);
    extensionTelescopio = lerp(5);
    largoCableActual = lerp(6);
    anguloPenduloX = lerp(7);
    anguloPenduloZ = lerp(8);

    baseY = lerp(9);
    hombroX = lerp(10);
    hombroZ = lerp(11);
    codo = lerp(12);
    munecaX = lerp(13);
    munecaZ = lerp(14);
    flexPulgar = lerp(15);
    flexIndice = lerp(16);
    flexMedio = lerp(17);
    flexAnular = lerp(18);
    flexMenique = lerp(19);

    i_curr_steps++;
  }
}

// =============================================================================
// SISTEMA DE ANIMACION 2: PERSONAJE (elder_Female) CAMINATA
// =============================================================================
typedef struct _char_frame {
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
} CHAR_FRAME;

#define CHAR_MAX_FRAMES 150
#define NUM_CHARS 1

CHAR_FRAME CharKeyFrame[NUM_CHARS][CHAR_MAX_FRAMES];
int CharFrameIndex[NUM_CHARS] = {0};
bool charPlay[NUM_CHARS] = {false};
int charPlayIndex[NUM_CHARS] = {0};
int char_i_curr_steps[NUM_CHARS] = {0};
int char_i_max_steps[NUM_CHARS];

float c_posX[NUM_CHARS], c_posY[NUM_CHARS], c_posZ[NUM_CHARS],
    c_rotY[NUM_CHARS];
float c_rTor[NUM_CHARS], c_rCab[NUM_CHARS];
float c_rBD[NUM_CHARS], c_rAD[NUM_CHARS];
float c_rBI[NUM_CHARS], c_rAI[NUM_CHARS];
float c_rPel[NUM_CHARS];
float c_rPD[NUM_CHARS], c_rTD[NUM_CHARS], c_rPieD[NUM_CHARS];
float c_rPI[NUM_CHARS], c_rTI[NUM_CHARS], c_rPieI[NUM_CHARS];

// Switches independientes de animaciones
bool gruaBrazoAnimOn = true;   // Controla grua torre + brazo robotico
bool personajeAnimOn = true;   // Controla personaje (caminata)
bool helicopteroAnimOn = true; // Controla helicoptero

// Variables de animacion del helicoptero
float rotHelice = 0.0f;
float rotCola = 0.0f;
float helicopteroHover = 0.0f;
float helicopteroHoverDir = 1.0f;

struct Waypoint {
  float x, z, angle;
};

Waypoint route[] = {
    {0.0f, 10.0f, 90.0f},  {3.5f, 8.5f, 135.0f},  {6.0f, 6.0f, 180.0f},
    {3.5f, 3.5f, 225.0f},  {0.0f, 2.0f, 270.0f},  {-3.5f, 3.5f, 315.0f},
    {-6.0f, 6.0f, 360.0f}, {-3.5f, 8.5f, 405.0f}, {0.0f, 10.0f, 450.0f}};
const int ROUTE_SIZE = 9;

void BuildWalkCycle(int c, float phaseOffset) {
  const float WALK_Y = 1.15f;

  float wx = route[0].x, wz = route[0].z;
  c_posX[c] = wx + phaseOffset * 0.3f;
  c_posY[c] = WALK_Y;
  c_posZ[c] = wz;
  c_rotY[c] = route[0].angle;
  c_rTor[c] = 0;
  c_rCab[c] = 5;
  c_rPel[c] = 0;
  c_rBD[c] = 0;
  c_rAD[c] = 0;
  c_rBI[c] = 0;
  c_rAI[c] = 0;
  c_rPD[c] = 0;
  c_rTD[c] = 0;
  c_rPieD[c] = 0;
  c_rPI[c] = 0;
  c_rTI[c] = 0;
  c_rPieI[c] = 0;

  CharFrameIndex[c] = 0;

#define SAVE_KF(ci)                                                            \
  do {                                                                         \
    CharKeyFrame[ci][CharFrameIndex[ci]].posX = c_posX[ci];                    \
    CharKeyFrame[ci][CharFrameIndex[ci]].posY = c_posY[ci];                    \
    CharKeyFrame[ci][CharFrameIndex[ci]].posZ = c_posZ[ci];                    \
    CharKeyFrame[ci][CharFrameIndex[ci]].rotY = c_rotY[ci];                    \
    CharKeyFrame[ci][CharFrameIndex[ci]].rTor = c_rTor[ci];                    \
    CharKeyFrame[ci][CharFrameIndex[ci]].rCab = c_rCab[ci];                    \
    CharKeyFrame[ci][CharFrameIndex[ci]].rPel = c_rPel[ci];                    \
    CharKeyFrame[ci][CharFrameIndex[ci]].rBD = c_rBD[ci];                      \
    CharKeyFrame[ci][CharFrameIndex[ci]].rAD = c_rAD[ci];                      \
    CharKeyFrame[ci][CharFrameIndex[ci]].rBI = c_rBI[ci];                      \
    CharKeyFrame[ci][CharFrameIndex[ci]].rAI = c_rAI[ci];                      \
    CharKeyFrame[ci][CharFrameIndex[ci]].rPD = c_rPD[ci];                      \
    CharKeyFrame[ci][CharFrameIndex[ci]].rTD = c_rTD[ci];                      \
    CharKeyFrame[ci][CharFrameIndex[ci]].rPieD = c_rPieD[ci];                  \
    CharKeyFrame[ci][CharFrameIndex[ci]].rPI = c_rPI[ci];                      \
    CharKeyFrame[ci][CharFrameIndex[ci]].rTI = c_rTI[ci];                      \
    CharKeyFrame[ci][CharFrameIndex[ci]].rPieI = c_rPieI[ci];                  \
    CharFrameIndex[ci]++;                                                      \
  } while (0)

  SAVE_KF(c);

  for (int seg = 0; seg < ROUTE_SIZE - 1; seg++) {
    float ax = route[seg].x, az = route[seg].z, aa = route[seg].angle;
    float bx = route[seg + 1].x, bz = route[seg + 1].z,
          ba = route[seg + 1].angle;

    float deltaAngle = ba - aa;
    while (deltaAngle > 180.0f)
      deltaAngle -= 360.0f;
    while (deltaAngle < -180.0f)
      deltaAngle += 360.0f;
    float leanTor = deltaAngle * 0.08f;

    for (int sub = 1; sub <= 4; sub++) {
      float t = sub / 4.0f;
      float mx = ax + (bx - ax) * t;
      float mz = az + (bz - az) * t;
      float ma = aa + (ba - aa) * t;

      bool rightLead = ((seg * 4 + sub + (int)(phaseOffset * 2)) % 2 == 0);

      c_posX[c] = mx;
      c_posY[c] = WALK_Y;
      c_posZ[c] = mz;
      c_rotY[c] = ma;
      c_rTor[c] = 3.0f;
      c_rCab[c] = 5.0f;
      c_rPel[c] = 2.0f + leanTor * 0.3f;

      if (rightLead) {
        c_rPD[c] = -25.0f;
        c_rTD[c] = 10.0f;
        c_rPieD[c] = -5.0f;
        c_rPI[c] = 20.0f;
        c_rTI[c] = 5.0f;
        c_rPieI[c] = 5.0f;
        c_rBI[c] = 12.0f + leanTor * 0.5f;
        c_rAI[c] = 0.0f;
        c_rBD[c] = -12.0f - leanTor * 0.5f;
        c_rAD[c] = 0.0f;
      } else {
        c_rPI[c] = -25.0f;
        c_rTI[c] = 10.0f;
        c_rPieI[c] = -5.0f;
        c_rPD[c] = 20.0f;
        c_rTD[c] = 5.0f;
        c_rPieD[c] = 5.0f;
        c_rBD[c] = 12.0f + leanTor * 0.5f;
        c_rAD[c] = 0.0f;
        c_rBI[c] = -12.0f - leanTor * 0.5f;
        c_rAI[c] = 0.0f;
      }

      SAVE_KF(c);
    }
  }

#undef SAVE_KF

  for (int i = 0; i < CharFrameIndex[c]; i++) {
    while (CharKeyFrame[c][i].rotY > 180.0f)
      CharKeyFrame[c][i].rotY -= 360.0f;
    while (CharKeyFrame[c][i].rotY < -180.0f)
      CharKeyFrame[c][i].rotY += 360.0f;
  }

  printf("[Char %d] %d keyframes generados (fase=%.1f)\n", c, CharFrameIndex[c],
         phaseOffset);
}

void interpolation(int c) {
  int p = charPlayIndex[c];

  float dx = CharKeyFrame[c][p + 1].posX - CharKeyFrame[c][p].posX;
  float dz = CharKeyFrame[c][p + 1].posZ - CharKeyFrame[c][p].posZ;
  float dist = sqrtf(dx * dx + dz * dz);
  int steps = (int)(dist * 400.0f);
  if (steps < 200)
    steps = 200;
  char_i_max_steps[c] = steps;

#define LERP(field, inc_field)                                                 \
  CharKeyFrame[c][p].inc_field =                                               \
      (CharKeyFrame[c][p + 1].field - CharKeyFrame[c][p].field) / steps

  LERP(posX, incX);
  LERP(posY, incY);
  LERP(posZ, incZ);

  float fromA = CharKeyFrame[c][p].rotY;
  float toA = CharKeyFrame[c][p + 1].rotY;
  float diff = toA - fromA;
  while (diff > 180.0f)
    diff -= 360.0f;
  while (diff < -180.0f)
    diff += 360.0f;
  CharKeyFrame[c][p].incRotY = diff / steps;

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
  c_posX[c] = CharKeyFrame[c][0].posX;
  c_posY[c] = CharKeyFrame[c][0].posY;
  c_posZ[c] = CharKeyFrame[c][0].posZ;
  c_rotY[c] = CharKeyFrame[c][0].rotY;
  c_rTor[c] = CharKeyFrame[c][0].rTor;
  c_rCab[c] = CharKeyFrame[c][0].rCab;
  c_rPel[c] = CharKeyFrame[c][0].rPel;
  c_rBD[c] = CharKeyFrame[c][0].rBD;
  c_rAD[c] = CharKeyFrame[c][0].rAD;
  c_rBI[c] = CharKeyFrame[c][0].rBI;
  c_rAI[c] = CharKeyFrame[c][0].rAI;
  c_rPD[c] = CharKeyFrame[c][0].rPD;
  c_rTD[c] = CharKeyFrame[c][0].rTD;
  c_rPieD[c] = CharKeyFrame[c][0].rPieD;
  c_rPI[c] = CharKeyFrame[c][0].rPI;
  c_rTI[c] = CharKeyFrame[c][0].rTI;
  c_rPieI[c] = CharKeyFrame[c][0].rPieI;
}

void CharAnimation() {
  for (int c = 0; c < NUM_CHARS; c++) {
    if (!charPlay[c])
      continue;

    if (char_i_curr_steps[c] >= char_i_max_steps[c]) {
      charPlayIndex[c]++;
      if (charPlayIndex[c] > CharFrameIndex[c] - 2) {
        charPlayIndex[c] = 0;
        char_i_curr_steps[c] = 0;
        resetChar(c);
        interpolation(c);
      } else {
        char_i_curr_steps[c] = 0;
        interpolation(c);
      }
    } else {
      int p = charPlayIndex[c];
      c_posX[c] += CharKeyFrame[c][p].incX;
      c_posY[c] += CharKeyFrame[c][p].incY;
      c_posZ[c] += CharKeyFrame[c][p].incZ;
      c_rotY[c] += CharKeyFrame[c][p].incRotY;
      while (c_rotY[c] > 180.0f)
        c_rotY[c] -= 360.0f;
      while (c_rotY[c] < -180.0f)
        c_rotY[c] += 360.0f;
      c_rTor[c] += CharKeyFrame[c][p].iTor;
      c_rCab[c] += CharKeyFrame[c][p].iCab;
      c_rPel[c] += CharKeyFrame[c][p].iPel;
      c_rBD[c] += CharKeyFrame[c][p].iBD;
      c_rAD[c] += CharKeyFrame[c][p].iAD;
      c_rBI[c] += CharKeyFrame[c][p].iBI;
      c_rAI[c] += CharKeyFrame[c][p].iAI;
      c_rPD[c] += CharKeyFrame[c][p].iPD;
      c_rTD[c] += CharKeyFrame[c][p].iTD;
      c_rPieD[c] += CharKeyFrame[c][p].iPieD;
      c_rPI[c] += CharKeyFrame[c][p].iPI;
      c_rTI[c] += CharKeyFrame[c][p].iTI;
      c_rPieI[c] += CharKeyFrame[c][p].iPieI;
      char_i_curr_steps[c]++;
    }
  }
}

// ---------------------------------------------------------------------------
// DrawCharacter
// Dibuja al personaje articulado (elder_Female) en su posicion actual.
// ---------------------------------------------------------------------------
void DrawCharacter(int c, Shader &shader, GLint mLoc, Model &Torso,
                   Model &Pelvis, Model &Cabeza, Model &TB_D, Model &TB_I,
                   Model &P_D, Model &T_D, Model &Pie_D, Model &P_I, Model &T_I,
                   Model &Pie_I) {
  glm::mat4 mG = glm::translate(glm::mat4(1.0f),
                                glm::vec3(c_posX[c], c_posY[c], c_posZ[c]));
  mG = glm::rotate(mG, glm::radians(c_rotY[c]), glm::vec3(0, 1, 0));
  mG = glm::scale(mG, glm::vec3(0.65f));

  // --- Torso ---
  glm::mat4 mT = glm::rotate(mG, glm::radians(c_rTor[c]), glm::vec3(1, 0, 0));
  mT = glm::translate(mT, glm::vec3(0.0f, 0.2f, 0.0f));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mT));
  Torso.Draw(shader);

  // --- Cabeza ---
  glm::mat4 mC = glm::translate(mT, glm::vec3(0.0f, 0.44f, 0.0f));
  mC = glm::rotate(mC, glm::radians(c_rCab[c]), glm::vec3(1, 0, 0));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mC));
  Cabeza.Draw(shader);

  // ================= BRAZO DERECHO =================
  glm::mat4 mBD = glm::translate(mT, glm::vec3(-0.10f, -0.55f, -0.08f));
  mBD = glm::rotate(mBD, glm::radians(42.0f), glm::vec3(0, 1, 1));
  mBD = glm::translate(mBD, glm::vec3(0.0f, 0.0f, 0.2f));
  mBD = glm::translate(mBD, glm::vec3(0.50f, 0.0f, 0.0f));
  mBD = glm::rotate(mBD, glm::radians(-90.0f), glm::vec3(0, 0, 1));
  mBD = glm::rotate(mBD, glm::radians(c_rBD[c]), glm::vec3(1, 0, 0));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mBD));
  TB_D.Draw(shader);

  // ================= BRAZO IZQUIERDO =================
  glm::mat4 mBI = glm::translate(mT, glm::vec3(0.10f, -0.50f, 0.1f));
  mBI = glm::rotate(mBI, glm::radians(-42.0f), glm::vec3(0, 1, 1));
  mBI = glm::translate(mBI, glm::vec3(-0.50f, 0.0f, 0.0f));
  mBI = glm::rotate(mBI, glm::radians(90.0f), glm::vec3(0, 0, 1));
  mBI = glm::rotate(mBI, glm::radians(c_rBI[c]), glm::vec3(1, 0, 0));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mBI));
  TB_I.Draw(shader);

  // --- Pelvis ---
  glm::mat4 mPel = glm::translate(mG, glm::vec3(0.0f, 0.0f, 0.0f));
  mPel = glm::rotate(mPel, glm::radians(c_rPel[c]), glm::vec3(0, 1, 0));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPel));
  Pelvis.Draw(shader);

  // --- Pierna Derecha ---
  glm::mat4 mPD = glm::translate(mPel, glm::vec3(-0.15f, -0.46f, 0.0f));
  mPD = glm::rotate(mPD, glm::radians(c_rPD[c]), glm::vec3(1, 0, 0));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPD));
  P_D.Draw(shader);

  glm::mat4 mTD = glm::translate(mPD, glm::vec3(0.0f, -0.27f, 0.0f));
  mTD = glm::rotate(mTD, glm::radians(c_rTD[c]), glm::vec3(1, 0, 0));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mTD));
  T_D.Draw(shader);

  glm::mat4 mPieD = glm::translate(mTD, glm::vec3(0.0f, -0.49f, 0.05f));
  mPieD = glm::rotate(mPieD, glm::radians(c_rPieD[c]), glm::vec3(1, 0, 0));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPieD));
  Pie_D.Draw(shader);

  // --- Pierna Izquierda ---
  glm::mat4 mPI = glm::translate(mPel, glm::vec3(0.15f, -0.46f, 0.0f));
  mPI = glm::rotate(mPI, glm::radians(c_rPI[c]), glm::vec3(1, 0, 0));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPI));
  P_I.Draw(shader);

  glm::mat4 mTI = glm::translate(mPI, glm::vec3(0.0f, -0.27f, 0.0f));
  mTI = glm::rotate(mTI, glm::radians(c_rTI[c]), glm::vec3(1, 0, 0));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mTI));
  T_I.Draw(shader);

  glm::mat4 mPieI = glm::translate(mTI, glm::vec3(0.0f, -0.49f, 0.05f));
  mPieI = glm::rotate(mPieI, glm::radians(c_rPieI[c]), glm::vec3(1, 0, 0));
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPieI));
  Pie_I.Draw(shader);
}

// =============================================================================
// VERTICES PARA FIGURAS BASICAS (cubo)
// =============================================================================
float vertices[] = {
    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.5f,  -0.5f, -0.5f,
    0.0f,  0.0f,  -1.0f, 0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
    0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, -0.5f, 0.5f,  -0.5f,
    0.0f,  0.0f,  -1.0f, -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,

    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,
    0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  -0.5f, 0.5f,  0.5f,
    0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,

    -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f,
    -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, 0.5f,
    -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,

    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
    1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
    0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, 0.5f,
    1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, -0.5f,
    0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
    0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, 0.5f,
    0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,

    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
    0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,
    0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f};

GLfloat skyboxVertices[] = {
    -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
    -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

    1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
};

// =============================================================================
// MAIN
// =============================================================================
int main() {
  // Inicializar memoria de keyframes del personaje y construir el ciclo
  // de caminata proceduralmente (sin archivos, sin terminal).
  for (int c = 0; c < NUM_CHARS; c++)
    for (int i = 0; i < CHAR_MAX_FRAMES; i++)
      memset(&CharKeyFrame[c][i], 0, sizeof(CHAR_FRAME));

  for (int c = 0; c < NUM_CHARS; c++)
    char_i_max_steps[c] = 1500;
  BuildWalkCycle(0, 0.0f);

  for (int c = 0; c < NUM_CHARS; c++) {
    if (CharFrameIndex[c] > 1) {
      resetChar(c);
      charPlayIndex[c] = 0;
      char_i_curr_steps[c] = 0;
      interpolation(c);
      charPlay[c] = true;
    }
  }

  // ---------- GLFW ----------
  glfwInit();
  GLFWwindow *window = glfwCreateWindow(
      WIDTH, HEIGHT, "Proyecto Final CGIHC - Integrado", nullptr, nullptr);
  if (nullptr == window) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
  glfwSetKeyCallback(window, KeyCallback);
  glfwSetCursorPosCallback(window, MouseCallback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glewExperimental = GL_TRUE;
  glewInit();
  glGetError();

  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  glEnable(GL_DEPTH_TEST);

  // ---------- Shaders ----------
  Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
  Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");
  Shader skyboxShader("Shader/SkyBox.vs", "Shader/SkyBox.frag");

  // ---------- Modelos de la escena ----------
  Model Escena((char *)"Models/trolleo.obj");
  Model Stands((char *)"Models/stands.obj");
  Model Lampara1((char *)"Models/lampara1.obj");
  Model Lampara2((char *)"Models/lampara2.obj");

  // ---------- Modelos del helicoptero ----------
  Model HelicCuerpo((char *)"Models/helicopteroCuerpo.obj");
  Model HelicHelice((char *)"Models/helicopteroHelice.obj");
  Model HelicCola((char *)"Models/helicopteroCola.obj");

  // ---------- Modelos del personaje ----------
  Model Torso((char *)"Models/mujer1/elder_Female_torso.obj");
  Model Pelvis((char *)"Models/mujer1/elder_Female_pelvis.obj");
  Model Cabeza((char *)"Models/mujer1/elder_Female_cabeza.obj");
  Model TB_D((char *)"Models/mujer1/elder_Female_tbrazo_d.obj");
  Model TB_I((char *)"Models/mujer1/elder_Female_tbrazo_i.obj");
  Model P_D((char *)"Models/mujer1/elder_Female_pierna_d.obj");
  Model T_D((char *)"Models/mujer1/elder_Female_tobillo_d.obj");
  Model Pie_D((char *)"Models/mujer1/elder_Female_pie_d.obj");
  Model P_I((char *)"Models/mujer1/elder_Female_pierna_i.obj");
  Model T_I((char *)"Models/mujer1/elder_Female_tobillo_i.obj");
  Model Pie_I((char *)"Models/mujer1/elder_Female_pie_i.obj");

  // ---------- VAO/VBO para cubo (usado por grua/brazo procedural) ----------
  GLuint indices[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                      12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                      24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};

  GLuint VBO, VAO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
                        (GLvoid *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // ---------- Skybox ----------
  GLuint skyboxVAO, skyboxVBO;
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);
  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                        (GLvoid *)0);
  glBindVertexArray(0);

  vector<const GLchar *> faces;
  faces.push_back("SkyBox/right.jpg");
  faces.push_back("SkyBox/left.jpg");
  faces.push_back("SkyBox/top.jpg");
  faces.push_back("SkyBox/bottom.jpg");
  faces.push_back("SkyBox/back.jpg");
  faces.push_back("SkyBox/front.jpg");
  GLuint cubemapTexture = TextureLoading::LoadCubemap(faces);

  lightingShader.Use();
  glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.difuse"),
              0);
  glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.specular"),
              1);

  glm::mat4 projection = glm::perspective(
      camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f,
      100.0f);

  // ---------- Lambdas de dibujo ----------
  auto DibujarGeometria = [&](glm::mat4 mBase, glm::vec3 escala,
                              glm::vec3 color, GLint modelLoc) {
    glm::mat4 mForma = glm::scale(mBase, escala);
    glUniform3fv(glGetUniformLocation(lightingShader.Program, "objectColor"), 1,
                 glm::value_ptr(color));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mForma));
    glDrawArrays(GL_TRIANGLES, 0, 36);
  };

  auto DibujarDedoDetallado = [&](glm::mat4 baseDedo, float flexion,
                                  float offsetX, float offsetY,
                                  float scaleGrosor, float scaleLargo,
                                  GLint modelLoc) {
    glm::mat4 mDedo = baseDedo;
    glm::vec3 colorNudillo(0.15f, 0.15f, 0.15f);
    glm::vec3 colorFalange(0.8f, 0.8f, 0.8f);

    mDedo = glm::translate(mDedo, glm::vec3(offsetX, offsetY, 0.0f));
    mDedo =
        glm::rotate(mDedo, glm::radians(flexion), glm::vec3(1.0f, 0.0f, 0.0f));
    DibujarGeometria(mDedo, glm::vec3(scaleGrosor * 1.2f), colorNudillo,
                     modelLoc);

    glm::mat4 mRender1 =
        glm::translate(mDedo, glm::vec3(0.0f, scaleLargo / 2, 0.0f));
    DibujarGeometria(mRender1, glm::vec3(scaleGrosor, scaleLargo, scaleGrosor),
                     colorFalange, modelLoc);

    mDedo = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo, 0.0f));
    mDedo = glm::rotate(mDedo, glm::radians(flexion * 0.8f),
                        glm::vec3(1.0f, 0.0f, 0.0f));
    DibujarGeometria(mDedo, glm::vec3(scaleGrosor * 1.1f), colorNudillo,
                     modelLoc);

    glm::mat4 mRender2 =
        glm::translate(mDedo, glm::vec3(0.0f, scaleLargo * 0.45f, 0.0f));
    DibujarGeometria(
        mRender2,
        glm::vec3(scaleGrosor * 0.9f, scaleLargo * 0.9f, scaleGrosor * 0.9f),
        colorFalange, modelLoc);

    mDedo = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo * 0.9f, 0.0f));
    mDedo = glm::rotate(mDedo, glm::radians(flexion * 0.6f),
                        glm::vec3(1.0f, 0.0f, 0.0f));
    DibujarGeometria(mDedo, glm::vec3(scaleGrosor * 0.9f), colorNudillo,
                     modelLoc);

    glm::mat4 mRender3 =
        glm::translate(mDedo, glm::vec3(0.0f, scaleLargo * 0.35f, 0.0f));
    DibujarGeometria(
        mRender3,
        glm::vec3(scaleGrosor * 0.8f, scaleLargo * 0.7f, scaleGrosor * 0.8f),
        colorFalange, modelLoc);
  };

  // --- Carga de animacion combinada de grua + brazo ---
  loadFromFile();

  // =========================================================================
  // BUCLE PRINCIPAL
  // =========================================================================
  while (!glfwWindowShouldClose(window)) {
    GLfloat currentFrame = (GLfloat)glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    glfwPollEvents();
    DoMovement();

    // Animaciones controladas por switches independientes
    if (gruaBrazoAnimOn) {
      Animation(); // grua + brazo
    }
    if (personajeAnimOn) {
      CharAnimation(); // personaje
    }
    if (helicopteroAnimOn) {
      helicopteroHover += helicopteroHoverDir * deltaTime * 0.25f;
      if (helicopteroHover > 0.15f)
        helicopteroHoverDir = -1.0f;
      if (helicopteroHover < -0.15f)
        helicopteroHoverDir = 1.0f;
      rotHelice += deltaTime * 600.0f;
      if (rotHelice >= 360.0f)
        rotHelice -= 360.0f;
      rotCola += deltaTime * 600.0f;
      if (rotCola >= 360.0f)
        rotCola -= 360.0f;
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    lightingShader.Use();

    GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
    glUniform3f(viewPosLoc, camera.GetPosition().x, camera.GetPosition().y,
                camera.GetPosition().z);

    glUniform3f(
        glGetUniformLocation(lightingShader.Program, "dirLight.direction"),
        0.0f, -1.0f, 0.0f);
    glUniform3f(
        glGetUniformLocation(lightingShader.Program, "dirLight.ambient"), 0.0f,
        0.0f, 0.0f);
    glUniform3f(
        glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), 0.0f,
        0.0f, 0.0f);
    glUniform3f(
        glGetUniformLocation(lightingShader.Program, "dirLight.specular"), 0.0f,
        0.0f, 0.0f);

    // 11 lamparas point lights
    float lA = lampsOn ? 0.05f : 0.0f;
    float lD = lampsOn ? 0.90f : 0.0f;
    float lS = lampsOn ? 0.50f : 0.0f;
    glm::vec3 allLampPos[11];
    for (int i = 0; i < 6; i++)
      allLampPos[i] = posLamp1[i];
    for (int i = 0; i < 5; i++)
      allLampPos[6 + i] = posLamp2[i];

    char buf[64];
    for (int i = 0; i < 11; i++) {
      snprintf(buf, sizeof(buf), "pointLights[%d].position", i);
      glUniform3f(glGetUniformLocation(lightingShader.Program, buf),
                  allLampPos[i].x, allLampPos[i].y - 0.5f, allLampPos[i].z);
      snprintf(buf, sizeof(buf), "pointLights[%d].ambient", i);
      glUniform3f(glGetUniformLocation(lightingShader.Program, buf), lA, lA,
                  lA);
      snprintf(buf, sizeof(buf), "pointLights[%d].diffuse", i);
      glUniform3f(glGetUniformLocation(lightingShader.Program, buf), lD,
                  lD * 0.95f, lD * 0.8f);
      snprintf(buf, sizeof(buf), "pointLights[%d].specular", i);
      glUniform3f(glGetUniformLocation(lightingShader.Program, buf), lS, lS,
                  lS);
      snprintf(buf, sizeof(buf), "pointLights[%d].constant", i);
      glUniform1f(glGetUniformLocation(lightingShader.Program, buf), 1.0f);
      snprintf(buf, sizeof(buf), "pointLights[%d].linear", i);
      glUniform1f(glGetUniformLocation(lightingShader.Program, buf), 0.14f);
      snprintf(buf, sizeof(buf), "pointLights[%d].quadratic", i);
      glUniform1f(glGetUniformLocation(lightingShader.Program, buf), 0.07f);
    }

    // Spotlight de la camara
    glUniform3f(
        glGetUniformLocation(lightingShader.Program, "spotLight.position"),
        camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
    glUniform3f(
        glGetUniformLocation(lightingShader.Program, "spotLight.direction"),
        camera.GetFront().x, camera.GetFront().y, camera.GetFront().z);
    glUniform3f(
        glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), 0.0f,
        0.0f, 0.0f);
    glUniform3f(
        glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), 1.0f,
        1.0f, 1.0f);
    glUniform3f(
        glGetUniformLocation(lightingShader.Program, "spotLight.specular"),
        1.0f, 1.0f, 1.0f);
    glUniform1f(
        glGetUniformLocation(lightingShader.Program, "spotLight.constant"),
        1.0f);
    glUniform1f(
        glGetUniformLocation(lightingShader.Program, "spotLight.linear"),
        0.09f);
    glUniform1f(
        glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"),
        0.032f);
    glUniform1f(
        glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"),
        glm::cos(glm::radians(12.5f)));
    glUniform1f(
        glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"),
        glm::cos(glm::radians(15.0f)));

    glUniform1f(
        glGetUniformLocation(lightingShader.Program, "material.shininess"),
        32.0f);

    glm::mat4 view = camera.GetViewMatrix();
    GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
    GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
    GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 model(1);

    // =====================================================================
    // ESCENA Y STANDS (CON TEXTURAS)
    // =====================================================================
    glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 1);

    // Escena - doble pase para transparencia
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"),
                1);
    Escena.Draw(lightingShader);

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"),
                2);
    Escena.Draw(lightingShader);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"),
                0);

    // Stands - doble pase para transparencia
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"),
                1);
    Stands.Draw(lightingShader);

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"),
                2);
    Stands.Draw(lightingShader);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"),
                0);

    // =====================================================================
    // PERSONAJE (CON TEXTURAS - mismas opciones que la escena)
    // =====================================================================
    glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 1);
    for (int c = 0; c < NUM_CHARS; c++) {
      DrawCharacter(c, lightingShader, modelLoc, Torso, Pelvis, Cabeza, TB_D,
                    TB_I, P_D, T_D, Pie_D, P_I, T_I, Pie_I);
    }

    // =====================================================================
    // HELICOPTERO (CON TEXTURAS, JERARQUICO)
    // =====================================================================
    glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 1);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"),
                0);

    // Cuerpo (padre) — posicion world + hover + rotacion 229.68° en Y (Blender
    // Z→OpenGL Y) Blender: (5.9861, -8.8815, 0.8229) → OpenGL: (X, Z_bl, -Y_bl)
    // = (5.9861, 0.8229, 8.8815)
    glm::vec3 helicopteroBase(5.9861f, 1.0729f + helicopteroHover, 8.8815f);
    glm::mat4 mHelicCuerpo = glm::translate(glm::mat4(1.0f), helicopteroBase);
    mHelicCuerpo = glm::rotate(mHelicCuerpo, glm::radians(229.68f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mHelicCuerpo));
    HelicCuerpo.Draw(lightingShader);

    // Helice principal (hijo del cuerpo) — gira sobre su propio eje Z (Blender)
    // = Y (OpenGL) Blender local: (-0.00001, -0.03831, -0.07575) OpenGL local:
    // (X_bl, Z_bl, -Y_bl) = (-0.00001, -0.07575, +0.03831)
    glm::vec3 heliceOffset(-0.00001f, 0.07575f,
                           0.03831f); // Y positivo = encima del cuerpo
    glm::mat4 mHelicHelice = glm::translate(mHelicCuerpo, heliceOffset);
    mHelicHelice = glm::rotate(mHelicHelice, glm::radians(rotHelice),
                               glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mHelicHelice));
    HelicHelice.Draw(lightingShader);

    // Helice de cola (hijo del cuerpo) — gira sobre su propio eje X (Blender) =
    // X local Blender local: (0.001558, 0.42116, 0.10811) OpenGL local:  (X_bl,
    // Z_bl, -Y_bl) = (0.001558, +0.10811, -0.42116)
    glm::vec3 colaOffset(0.001558f, 0.10811f, -0.42116f);
    glm::mat4 mHelicCola = glm::translate(mHelicCuerpo, colaOffset);
    mHelicCola = glm::rotate(mHelicCola, glm::radians(rotCola),
                             glm::vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mHelicCola));
    HelicCola.Draw(lightingShader);

    // =====================================================================
    // GRUA TORRE + BRAZO ROBOTICO (PROCEDURALES, SIN TEXTURAS)
    // =====================================================================
    glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 0);

    glBindVertexArray(VAO);
    std::stack<glm::mat4> pila;
    glm::vec3 colorGrua;

    // ---> GRUA TORRE <---
    glm::mat4 modelBaseGrua =
        glm::translate(glm::mat4(1.0f), glm::vec3(-8.5f, 0.7f, -9.3f));
    modelBaseGrua = glm::scale(modelBaseGrua, glm::vec3(0.125f));
    modelBaseGrua = glm::rotate(modelBaseGrua, glm::radians(90.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f));

    colorGrua = glm::vec3(0.3f, 0.3f, 0.3f);
    DibujarGeometria(modelBaseGrua, glm::vec3(3.0f, 0.5f, 3.0f), colorGrua,
                     modelLoc);

    int numSegmentos = 6;
    float altoSeg = 1.5f, anchoSeg = 1.2f, grosor = 0.1f;
    colorGrua = glm::vec3(0.9f, 0.7f, 0.1f);

    for (int i = 0; i < numSegmentos; i++) {
      glm::mat4 baseSeg = glm::translate(
          modelBaseGrua, glm::vec3(0.0f, 0.25f + (i * altoSeg), 0.0f));
      for (int x : {-1, 1}) {
        for (int z : {-1, 1}) {
          glm::mat4 poste =
              glm::translate(baseSeg, glm::vec3(x * anchoSeg / 2, altoSeg / 2,
                                                z * anchoSeg / 2));
          DibujarGeometria(poste, glm::vec3(grosor, altoSeg, grosor), colorGrua,
                           modelLoc);
        }
      }
      for (int z : {-1, 1}) {
        glm::mat4 horiz =
            glm::translate(baseSeg, glm::vec3(0.0f, altoSeg, z * anchoSeg / 2));
        DibujarGeometria(horiz, glm::vec3(anchoSeg + grosor, grosor, grosor),
                         colorGrua, modelLoc);
      }
      for (int x : {-1, 1}) {
        glm::mat4 horiz =
            glm::translate(baseSeg, glm::vec3(x * anchoSeg / 2, altoSeg, 0.0f));
        DibujarGeometria(horiz, glm::vec3(grosor, grosor, anchoSeg + grosor),
                         colorGrua, modelLoc);
      }
      float anguloDiag = atan2(altoSeg, anchoSeg);
      float largoDiag = sqrt(altoSeg * altoSeg + anchoSeg * anchoSeg);
      for (int z : {-1, 1}) {
        glm::mat4 diag = glm::translate(
            baseSeg, glm::vec3(0.0f, altoSeg / 2, z * anchoSeg / 2));
        diag = glm::rotate(diag, (z == 1) ? anguloDiag : -anguloDiag,
                           glm::vec3(0.0f, 0.0f, 1.0f));
        DibujarGeometria(diag, glm::vec3(largoDiag, grosor / 2, grosor / 2),
                         colorGrua, modelLoc);
      }
    }

    float alturaTorre = 0.25f + (numSegmentos * altoSeg);
    glm::mat4 modelTorreta = glm::translate(
        modelBaseGrua, glm::vec3(0.0f, alturaTorre + 0.2f, 0.0f));
    modelTorreta = glm::rotate(modelTorreta, glm::radians(rotTorreta),
                               glm::vec3(0.0f, 1.0f, 0.0f));
    pila.push(modelTorreta);

    DibujarGeometria(modelTorreta, glm::vec3(2.0f, 0.4f, 2.0f), glm::vec3(0.2f),
                     modelLoc);

    glm::mat4 renderCabina =
        glm::translate(modelTorreta, glm::vec3(-0.9f, -0.6f, 0.9f));
    DibujarGeometria(renderCabina, glm::vec3(0.8f, 1.5f, 1.0f),
                     glm::vec3(0.1f, 0.4f, 0.7f), modelLoc);

    glm::mat4 renderTrasero =
        glm::translate(modelTorreta, glm::vec3(0.0f, 0.3f, -1.2f));
    DibujarGeometria(renderTrasero, glm::vec3(1.5f, 0.8f, 1.5f),
                     glm::vec3(0.15f), modelLoc);

    glm::mat4 modelMastil =
        glm::translate(modelTorreta, glm::vec3(0.0f, 1.5f, 0.0f));
    pila.push(modelMastil);
    DibujarGeometria(modelMastil, glm::vec3(0.8f, 3.0f, 0.8f), colorGrua,
                     modelLoc);

    glm::mat4 modelPlumaBase = pila.top();
    modelPlumaBase =
        glm::translate(modelPlumaBase, glm::vec3(0.0f, 0.0f, 0.5f));
    modelPlumaBase = glm::rotate(modelPlumaBase, glm::radians(elevacionPluma),
                                 glm::vec3(1.0f, 0.0f, 0.0f));
    pila.push(modelPlumaBase);

    glm::mat4 renderPluma =
        glm::translate(modelPlumaBase, glm::vec3(0.0f, 0.0f, 4.0f));
    DibujarGeometria(renderPluma, glm::vec3(0.8f, 0.8f, 8.0f), colorGrua,
                     modelLoc);

    glm::mat4 modelTeleNode = pila.top();
    modelTeleNode = glm::translate(
        modelTeleNode, glm::vec3(0.0f, 0.0f, 1.0f + extensionTelescopio));
    pila.push(modelTeleNode);

    glm::mat4 renderTele =
        glm::translate(modelTeleNode, glm::vec3(0.0f, 0.0f, 4.0f));
    DibujarGeometria(renderTele, glm::vec3(0.6f, 0.6f, 8.0f), glm::vec3(0.7f),
                     modelLoc);

    glm::mat4 modelAnclajeGiro =
        glm::translate(modelTeleNode, glm::vec3(0.0f, -0.3f, 8.0f));
    pila.pop();
    pila.pop();
    pila.pop();
    pila.pop();

    modelAnclajeGiro =
        glm::rotate(modelAnclajeGiro, glm::radians(-elevacionPluma),
                    glm::vec3(1.0f, 0.0f, 0.0f));
    modelAnclajeGiro =
        glm::rotate(modelAnclajeGiro, glm::radians(anguloPenduloX),
                    glm::vec3(1.0f, 0.0f, 0.0f));
    modelAnclajeGiro =
        glm::rotate(modelAnclajeGiro, glm::radians(anguloPenduloZ),
                    glm::vec3(0.0f, 0.0f, 1.0f));

    float conWidth = 3.6f, conHeight = 1.2f, conDepth = 1.2f;
    float spreadX = conWidth / 2.0f;

    DibujarGeometria(modelAnclajeGiro, glm::vec3(0.4f), glm::vec3(0.1f),
                     modelLoc);

    float diagLength =
        sqrt((spreadX * spreadX) + (largoCableActual * largoCableActual));
    float angleCable = atan2(spreadX, largoCableActual);

    for (int i = -1; i <= 1; i += 2) {
      float xDir = (float)i;
      glm::mat4 renderCable = glm::translate(
          modelAnclajeGiro,
          glm::vec3(xDir * spreadX / 2.0f, -largoCableActual / 2.0f, 0.0f));
      renderCable = glm::rotate(renderCable, xDir * angleCable,
                                glm::vec3(0.0f, 0.0f, 1.0f));
      DibujarGeometria(renderCable, glm::vec3(0.04f, diagLength, 0.04f),
                       glm::vec3(0.0f), modelLoc);
    }

    glm::mat4 modelContenedorBase = glm::translate(
        modelAnclajeGiro, glm::vec3(0.0f, -largoCableActual - 0.6f, 0.0f));
    DibujarGeometria(modelContenedorBase, glm::vec3(3.6f, 1.2f, 1.2f),
                     glm::vec3(0.5f, 0.15f, 0.1f), modelLoc);

    int numRibs = 14;
    float ribGrosor = 0.06f, ribSaliente = 0.05f;
    glm::vec3 colorRib(0.4f, 0.1f, 0.05f);

    for (int i = 0; i < numRibs; i++) {
      float xPosRib = -conWidth / 2.0f + (i * conWidth / (numRibs - 1));
      glm::mat4 renderRibFrente = glm::translate(
          modelContenedorBase,
          glm::vec3(xPosRib, 0.0f, conDepth / 2.0f + ribSaliente / 2.0f));
      DibujarGeometria(renderRibFrente,
                       glm::vec3(ribGrosor, conHeight + 0.01f, ribSaliente),
                       colorRib, modelLoc);

      glm::mat4 renderRibAtras = glm::translate(
          modelContenedorBase,
          glm::vec3(xPosRib, 0.0f, -conDepth / 2.0f - ribSaliente / 2.0f));
      DibujarGeometria(renderRibAtras,
                       glm::vec3(ribGrosor, conHeight + 0.01f, ribSaliente),
                       colorRib, modelLoc);
    }

    // ---> BRAZO ROBOTICO Y PIEZA OBJETIVO <---
    glm::mat4 mPieza = glm::mat4(1.0f);
    mPieza = glm::translate(mPieza, glm::vec3(-6.5f, -4.5f, -3.0f));
    mPieza = glm::scale(mPieza, glm::vec3(0.125f));
    DibujarGeometria(mPieza, glm::vec3(1.2f), glm::vec3(0.9f, 0.2f, 0.2f),
                     modelLoc);

    glm::mat4 modelBaseBrazo =
        glm::translate(glm::mat4(1.0f), glm::vec3(-8.3f, 0.75f, -4.0f));
    modelBaseBrazo = glm::scale(modelBaseBrazo, glm::vec3(0.125f));

    glm::vec3 colorBrazo(0.6f, 0.6f, 0.65f);
    glm::vec3 colorArticulacion(0.15f, 0.15f, 0.15f);

    modelBaseBrazo = glm::rotate(modelBaseBrazo, glm::radians(baseY),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    pila.push(modelBaseBrazo);
    DibujarGeometria(modelBaseBrazo, glm::vec3(3.0f, 0.5f, 3.0f),
                     colorArticulacion, modelLoc);

    glm::mat4 modHombro =
        glm::translate(modelBaseBrazo, glm::vec3(0.0f, 1.0f, 0.0f));
    modHombro = glm::rotate(modHombro, glm::radians(hombroX),
                            glm::vec3(1.0f, 0.0f, 0.0f));
    modHombro = glm::rotate(modHombro, glm::radians(hombroZ),
                            glm::vec3(0.0f, 0.0f, 1.0f));
    pila.push(modHombro);
    DibujarGeometria(modHombro, glm::vec3(1.5f), colorArticulacion, modelLoc);

    glm::mat4 renderBrazo =
        glm::translate(modHombro, glm::vec3(0.0f, 2.0f, 0.0f));
    DibujarGeometria(renderBrazo, glm::vec3(1.2f, 4.0f, 1.2f), colorBrazo,
                     modelLoc);

    glm::mat4 modCodo = pila.top();
    modCodo = glm::translate(modCodo, glm::vec3(0.0f, 4.0f, 0.0f));
    modCodo =
        glm::rotate(modCodo, glm::radians(codo), glm::vec3(1.0f, 0.0f, 0.0f));
    pila.push(modCodo);
    DibujarGeometria(modCodo, glm::vec3(1.4f, 1.2f, 1.4f), colorArticulacion,
                     modelLoc);

    glm::mat4 barraIzq = glm::translate(modCodo, glm::vec3(-0.4f, 2.0f, 0.0f));
    DibujarGeometria(barraIzq, glm::vec3(0.3f, 4.0f, 0.8f), colorBrazo,
                     modelLoc);

    glm::mat4 barraDer = glm::translate(modCodo, glm::vec3(0.4f, 2.0f, 0.0f));
    DibujarGeometria(barraDer, glm::vec3(0.3f, 4.0f, 0.8f), colorBrazo,
                     modelLoc);

    glm::mat4 piston = glm::translate(modCodo, glm::vec3(0.0f, 2.0f, 0.0f));
    DibujarGeometria(piston, glm::vec3(0.2f, 3.8f, 0.2f), glm::vec3(0.2f),
                     modelLoc);

    glm::mat4 modMuneca = pila.top();
    modMuneca = glm::translate(modMuneca, glm::vec3(0.0f, 4.0f, 0.0f));
    modMuneca = glm::rotate(modMuneca, glm::radians(munecaX),
                            glm::vec3(1.0f, 0.0f, 0.0f));
    modMuneca = glm::rotate(modMuneca, glm::radians(munecaZ),
                            glm::vec3(0.0f, 0.0f, 1.0f));
    pila.push(modMuneca);
    DibujarGeometria(modMuneca, glm::vec3(1.2f, 0.8f, 1.2f), colorArticulacion,
                     modelLoc);

    glm::mat4 renderPalma =
        glm::translate(modMuneca, glm::vec3(0.0f, 1.0f, 0.0f));
    DibujarGeometria(renderPalma, glm::vec3(2.5f, 2.0f, 0.6f), glm::vec3(0.7f),
                     modelLoc);

    glm::mat4 soportePulgar =
        glm::translate(modMuneca, glm::vec3(-1.25f, -0.3f, 0.0f));
    DibujarGeometria(soportePulgar, glm::vec3(0.6f, 1.0f, 0.6f),
                     glm::vec3(0.7f), modelLoc);

    glm::mat4 baseMano = pila.top();

    DibujarDedoDetallado(baseMano, flexIndice, -0.9f, 2.0f, 0.35f, 1.0f,
                         modelLoc);
    DibujarDedoDetallado(baseMano, flexMedio, -0.3f, 2.0f, 0.38f, 1.1f,
                         modelLoc);
    DibujarDedoDetallado(baseMano, flexAnular, 0.3f, 2.0f, 0.35f, 1.0f,
                         modelLoc);
    DibujarDedoDetallado(baseMano, flexMenique, 0.9f, 2.0f, 0.30f, 0.8f,
                         modelLoc);

    glm::mat4 mPulgar = baseMano;
    mPulgar = glm::translate(mPulgar, glm::vec3(-1.3f, 0.2f, 0.0f));
    mPulgar =
        glm::rotate(mPulgar, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mPulgar =
        glm::rotate(mPulgar, glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    DibujarDedoDetallado(mPulgar, flexPulgar, 0.0f, 0.0f, 0.40f, 0.9f,
                         modelLoc);

    pila.pop();
    pila.pop();
    pila.pop();
    pila.pop();

    // =====================================================================
    // LAMPARAS (.obj)
    // =====================================================================
    glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 1);

    lampShader.Use();
    GLint lampModelLoc = glGetUniformLocation(lampShader.Program, "model");
    glUniformMatrix4fv(glGetUniformLocation(lampShader.Program, "view"), 1,
                       GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(lampShader.Program, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));

    for (int i = 0; i < 6; i++) {
      glm::mat4 modelLamp = glm::translate(glm::mat4(1.0f), posLamp1[i]);
      glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelLamp));
      Lampara1.Draw(lampShader);
    }

    glm::vec3 scL2Small(0.3135f, 0.1175f, 0.111f);
    for (int i = 0; i < 2; i++) {
      glm::mat4 modelLamp = glm::translate(glm::mat4(1.0f), posLamp2[i]);
      modelLamp = glm::scale(modelLamp, scL2Small);
      glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelLamp));
      Lampara2.Draw(lampShader);
    }
    for (int i = 2; i < 5; i++) {
      glm::mat4 modelLamp = glm::translate(glm::mat4(1.0f), posLamp2[i]);
      modelLamp = glm::rotate(modelLamp, glm::radians(90.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f));
      modelLamp = glm::scale(modelLamp, scL2Small);
      glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelLamp));
      Lampara2.Draw(lampShader);
    }

    glBindVertexArray(0);

    // =====================================================================
    // SKYBOX
    // =====================================================================
    glDepthFunc(GL_LEQUAL);
    skyboxShader.Use();
    glm::mat4 skyView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.Program, "view"), 1,
                       GL_FALSE, glm::value_ptr(skyView));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.Program, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);

    glfwSwapBuffers(window);
  }

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteVertexArrays(1, &skyboxVAO);
  glDeleteBuffers(1, &skyboxVBO);
  glfwTerminate();

  return 0;
}

// =============================================================================
// CONTROLES Y CALLBACKS
// =============================================================================
void DoMovement() {
  if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
    camera.ProcessKeyboard(RIGHT, deltaTime);
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                 int mode) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);

  // B: encender/apagar lamparas
  if (key == GLFW_KEY_B && action == GLFW_PRESS) {
    lampsOn = !lampsOn;
    std::cout << "Lamparas: " << (lampsOn ? "ON" : "OFF") << std::endl;
  }

  // G: encender/apagar animacion de GRUA + BRAZO
  if (key == GLFW_KEY_G && action == GLFW_PRESS) {
    gruaBrazoAnimOn = !gruaBrazoAnimOn;
    std::cout << "Animacion Grua+Brazo: " << (gruaBrazoAnimOn ? "ON" : "OFF")
              << std::endl;
  }

  // N: encender/apagar animacion de PERSONAJE
  if (key == GLFW_KEY_N && action == GLFW_PRESS) {
    personajeAnimOn = !personajeAnimOn;
    std::cout << "Animacion Personaje: " << (personajeAnimOn ? "ON" : "OFF")
              << std::endl;
  }

  // H: encender/apagar animacion del HELICOPTERO
  if (key == GLFW_KEY_H && action == GLFW_PRESS) {
    helicopteroAnimOn = !helicopteroAnimOn;
    std::cout << "Animacion Helicoptero: " << (helicopteroAnimOn ? "ON" : "OFF")
              << std::endl;
  }

  if (key >= 0 && key < 1024) {
    if (action == GLFW_PRESS)
      keys[key] = true;
    if (action == GLFW_RELEASE)
      keys[key] = false;
  }
}

void MouseCallback(GLFWwindow *window, double xPos, double yPos) {
  if (firstMouse) {
    lastX = (float)xPos;
    lastY = (float)yPos;
    firstMouse = false;
  }
  GLfloat xOffset = (float)(xPos - lastX);
  GLfloat yOffset = (float)(lastY - yPos);
  lastX = (float)xPos;
  lastY = (float)yPos;
  camera.ProcessMouseMovement(xOffset, yOffset);
}

// =============================================================================
// PROYECTO FINAL CGIHC - INTEGRADO
//   - Escena: lobby (trolleo.obj) + stands + lamparas + skybox
//   - Animacion 1 (grua torre + brazo robotico): se carga desde
//       Animations/animacion_grua.txt
//       Animations/animacion_pick_place.txt
//   - Animacion 2 (personaje elder_Female caminando): ciclo de caminata
//       generado proceduralmente al inicio (BuildWalkCycle).
//
//   Controles principales:
//     WASD / flechas : mover camara
//     raton          : mirar
//     B              : encender/apagar lamparas
//     G              : encender/apagar animacion de GRUA + BRAZO
//     N              : encender/apagar animacion de PERSONAJE
//     ESC            : salir
// =============================================================================

// #include <algorithm>
// #include <cmath>
// #include <cstring>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include <stack>
// #include <string>
// #include <vector>

// // GLEW / GLFW
// #include <GL/glew.h>
// #include <GLFW/glfw3.h>

// // Other libs
// #include "stb_image.h"
// #include "SOIL2/SOIL2.h"

// // GLM
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #include <glm/gtc/quaternion.hpp>
// #include <glm/ext/matrix_float4x4.hpp>
// #include <glm/ext/matrix_transform.hpp>
// #include <glm/ext/vector_float3.hpp>
// #include <glm/geometric.hpp>

// // Otros
// #include "Camera.h"
// #include "Model.h"
// #include "Shader.h"
// #include "Texture.h"

// using namespace std;

// #if defined(_WIN32)
// #define dir "Animations\\"
// #else
// #define dir "Animations/"
// #endif

// //
// =============================================================================
// // PROTOTIPOS
// //
// =============================================================================
// void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int
// mode); void MouseCallback(GLFWwindow* window, double xPos, double yPos); void
// DoMovement();

// // Animacion de la grua + brazo robotico (sistema "v[20]")
// void Animation();
// void resetElements();
// void loadFromFile();
// void applyFrame(int index);

// // Animacion del personaje (sistema "FRAME por campos")
// void CharAnimation();
// void interpolation(int c);
// void resetChar(int c);
// void BuildWalkCycle(int c, float phaseOffset);

// //
// =============================================================================
// // VENTANA Y CAMARA
// //
// =============================================================================
// const GLuint WIDTH = 1200, HEIGHT = 800;
// int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera camera(glm::vec3(0.0f, 5.0f, 20.0f));
// GLfloat lastX = WIDTH / 2.0f;
// GLfloat lastY = HEIGHT / 2.0f;
// bool keys[1024];
// bool firstMouse = true;
// bool lampsOn = true;

// GLfloat deltaTime = 0.0f;
// GLfloat lastFrame = 0.0f;

// //
// =============================================================================
// // LAMPARAS (posiciones en mundo)
// //
// =============================================================================
// glm::vec3 posLamp1[6] = {
//     {0.0000f, 3.66f, -3.4804f}, {6.4402f, 3.66f, -3.4804f},
//     {6.4402f, 3.66f,  2.7842f}, {0.0000f, 3.66f,  2.7842f},
//     {-7.3793f, 3.66f, 4.9873f}, {-7.3793f, 3.66f, 1.0176f},
// };
// glm::vec3 posLamp2[5] = {
//     { 1.1049f, 3.66f, 11.2860f}, {-1.5901f, 3.66f, 11.2860f},
//     {-7.3817f, 3.66f,  8.7838f}, {-7.3817f, 3.66f, -3.5014f},
//     {-7.3817f, 3.66f, -8.6545f},
// };

// //
// =============================================================================
// // SISTEMA DE ANIMACION 1: GRUA TORRE + BRAZO ROBOTICO
// //
// =============================================================================
// float gruaPosX = 0.0f, gruaPosY = 0.0f, gruaPosZ = 0.0f;
// float rotTorreta = 0.0f, elevacionPluma = 25.0f, extensionTelescopio = 0.0f,
// largoCableActual = 6.0f; float anguloPenduloX = 0.0f, anguloPenduloZ = 0.0f;

// float baseY = 0.0f, hombroX = 0.0f, hombroZ = 0.0f, codo = 0.0f, munecaX =
// 0.0f, munecaZ = 0.0f; float flexPulgar = 0.0f, flexIndice = 0.0f, flexMedio =
// 0.0f, flexAnular = 0.0f, flexMenique = 0.0f;

// #define ANIM_VARS 20
// #define MAX_FRAMES 150
// int i_max_steps = 150;
// int i_curr_steps = 0;

// typedef struct _frame {
//     float v[ANIM_VARS];
// } FRAME;

// FRAME KeyFrame[MAX_FRAMES];
// int FrameIndex = 0;
// bool play = false;
// int playIndex = 0;

// void applyFrame(int index) {
//     gruaPosX = KeyFrame[index].v[0];
//     gruaPosY = KeyFrame[index].v[1];
//     gruaPosZ = KeyFrame[index].v[2];
//     rotTorreta = KeyFrame[index].v[3];
//     elevacionPluma = KeyFrame[index].v[4];
//     extensionTelescopio = KeyFrame[index].v[5];
//     largoCableActual = KeyFrame[index].v[6];
//     anguloPenduloX = KeyFrame[index].v[7];
//     anguloPenduloZ = KeyFrame[index].v[8];

//     baseY = KeyFrame[index].v[9];
//     hombroX = KeyFrame[index].v[10];
//     hombroZ = KeyFrame[index].v[11];
//     codo = KeyFrame[index].v[12];
//     munecaX = KeyFrame[index].v[13];
//     munecaZ = KeyFrame[index].v[14];
//     flexPulgar = KeyFrame[index].v[15];
//     flexIndice = KeyFrame[index].v[16];
//     flexMedio = KeyFrame[index].v[17];
//     flexAnular = KeyFrame[index].v[18];
//     flexMenique = KeyFrame[index].v[19];
// }

// void resetElements(void) {
//     if (FrameIndex > 0) applyFrame(0);
// }

// void loadFromFile() {
//     ifstream fGrua(dir + string("animacion_grua.txt"));
//     ifstream fBrazo(dir + string("animacion_pick_place.txt"));

//     int framesGrua = 0;
//     int framesBrazo = 0;

//     if (fGrua.is_open()) fGrua >> framesGrua;
//     if (fBrazo.is_open()) fBrazo >> framesBrazo;

//     FrameIndex = max(framesGrua, framesBrazo);
//     if (FrameIndex > MAX_FRAMES) FrameIndex = MAX_FRAMES;

//     for (int i = 0; i < FrameIndex; i++) {
//         for (int j = 0; j < ANIM_VARS; j++) KeyFrame[i].v[j] = 0.0f;
//         KeyFrame[i].v[4] = 25.0f; // elevacionPluma por defecto
//         KeyFrame[i].v[6] = 6.0f;  // largoCable por defecto

//         if (i < framesGrua && fGrua.is_open()) {
//             for (int j = 0; j < 9; j++) fGrua >> KeyFrame[i].v[j];
//         }
//         else if (i > 0) {
//             for (int j = 0; j < 9; j++) KeyFrame[i].v[j] = KeyFrame[i -
//             1].v[j];
//         }

//         if (i < framesBrazo && fBrazo.is_open()) {
//             for (int j = 9; j < 20; j++) fBrazo >> KeyFrame[i].v[j];
//         }
//         else if (i > 0) {
//             for (int j = 9; j < 20; j++) KeyFrame[i].v[j] = KeyFrame[i -
//             1].v[j];
//         }
//     }

//     if (fGrua.is_open()) fGrua.close();
//     if (fBrazo.is_open()) fBrazo.close();

//     if (FrameIndex > 1) {
//         cout << "[Grua/Brazo] Animaciones combinadas cargadas. Frames: " <<
//         FrameIndex << endl; play = true; resetElements();
//     }
//     else {
//         cout << "[Grua/Brazo] ADVERTENCIA: no se encontraron los archivos
//         .txt." << endl;
//     }
// }

// void Animation() {
//     if (!play || FrameIndex < 2) return;

//     if (i_curr_steps >= i_max_steps) {
//         playIndex++;
//         if (playIndex > FrameIndex - 2) {
//             playIndex = 0;
//             resetElements();
//         }
//         i_curr_steps = 0;
//     }
//     else {
//         float t = (float)i_curr_steps / (float)i_max_steps;
//         float smooth_t = t * t * (3.0f - 2.0f * t);

//         auto lerp = [&](int varIdx) {
//             return KeyFrame[playIndex].v[varIdx] +
//                 (KeyFrame[playIndex + 1].v[varIdx] -
//                 KeyFrame[playIndex].v[varIdx]) * smooth_t;
//             };

//         gruaPosX = lerp(0);
//         gruaPosY = lerp(1);
//         gruaPosZ = lerp(2);
//         rotTorreta = lerp(3);
//         elevacionPluma = lerp(4);
//         extensionTelescopio = lerp(5);
//         largoCableActual = lerp(6);
//         anguloPenduloX = lerp(7);
//         anguloPenduloZ = lerp(8);

//         baseY = lerp(9);
//         hombroX = lerp(10);
//         hombroZ = lerp(11);
//         codo = lerp(12);
//         munecaX = lerp(13);
//         munecaZ = lerp(14);
//         flexPulgar = lerp(15);
//         flexIndice = lerp(16);
//         flexMedio = lerp(17);
//         flexAnular = lerp(18);
//         flexMenique = lerp(19);

//         i_curr_steps++;
//     }
// }

// //
// =============================================================================
// // SISTEMA DE ANIMACION 2: PERSONAJE (elder_Female) CAMINATA
// //
// =============================================================================
// typedef struct _char_frame {
//     float posX, posY, posZ;
//     float rotY;
//     float incX, incY, incZ, incRotY;
//     float rTor;
//     float rCab;
//     float rBD, rAD;
//     float rBI, rAI;
//     float rPel;
//     float rPD, rTD, rPieD;
//     float rPI, rTI, rPieI;
//     float iTor, iCab;
//     float iBD, iAD, iBI, iAI;
//     float iPel;
//     float iPD, iTD, iPieD;
//     float iPI, iTI, iPieI;
// } CHAR_FRAME;

// #define CHAR_MAX_FRAMES 150
// #define NUM_CHARS  1

// CHAR_FRAME CharKeyFrame[NUM_CHARS][CHAR_MAX_FRAMES];
// int    CharFrameIndex[NUM_CHARS] = { 0 };
// bool   charPlay[NUM_CHARS] = { false };
// int    charPlayIndex[NUM_CHARS] = { 0 };
// int    char_i_curr_steps[NUM_CHARS] = { 0 };
// int    char_i_max_steps[NUM_CHARS];

// float  c_posX[NUM_CHARS], c_posY[NUM_CHARS], c_posZ[NUM_CHARS],
// c_rotY[NUM_CHARS]; float  c_rTor[NUM_CHARS], c_rCab[NUM_CHARS]; float
// c_rBD[NUM_CHARS], c_rAD[NUM_CHARS]; float  c_rBI[NUM_CHARS],
// c_rAI[NUM_CHARS]; float  c_rPel[NUM_CHARS]; float  c_rPD[NUM_CHARS],
// c_rTD[NUM_CHARS], c_rPieD[NUM_CHARS]; float  c_rPI[NUM_CHARS],
// c_rTI[NUM_CHARS], c_rPieI[NUM_CHARS];

// // Switches independientes de animaciones
// bool gruaBrazoAnimOn = true;   // Controla grua torre + brazo robotico
// bool personajeAnimOn = true;   // Controla personaje (caminata)
// bool helicopteroAnimOn = true; // Controla helicoptero

// // Variables de animacion del helicoptero
// float rotHelice = 0.0f;
// float rotCola   = 0.0f;
// float helicopteroHover    = 0.0f;
// float helicopteroHoverDir = 1.0f;

// struct Waypoint { float x, z, angle; };

// Waypoint route[] = {
//     {  0.0f,  10.0f,  90.0f },
//     {  3.5f,   8.5f, 135.0f },
//     {  6.0f,   6.0f, 180.0f },
//     {  3.5f,   3.5f, 225.0f },
//     {  0.0f,   2.0f, 270.0f },
//     { -3.5f,   3.5f, 315.0f },
//     { -6.0f,   6.0f, 360.0f },
//     { -3.5f,   8.5f, 405.0f },
//     {  0.0f,  10.0f, 450.0f }
// };
// const int ROUTE_SIZE = 9;

// void BuildWalkCycle(int c, float phaseOffset) {
//     const float WALK_Y = 1.15f;

//     float wx = route[0].x, wz = route[0].z;
//     c_posX[c] = wx + phaseOffset * 0.3f;
//     c_posY[c] = WALK_Y;
//     c_posZ[c] = wz;
//     c_rotY[c] = route[0].angle;
//     c_rTor[c] = 0; c_rCab[c] = 5; c_rPel[c] = 0;
//     c_rBD[c] = 0;  c_rAD[c] = 0;  c_rBI[c] = 0; c_rAI[c] = 0;
//     c_rPD[c] = 0;  c_rTD[c] = 0;  c_rPieD[c] = 0;
//     c_rPI[c] = 0;  c_rTI[c] = 0;  c_rPieI[c] = 0;

//     CharFrameIndex[c] = 0;

// #define SAVE_KF(ci) do { \
//         CharKeyFrame[ci][CharFrameIndex[ci]].posX  = c_posX[ci]; \
//         CharKeyFrame[ci][CharFrameIndex[ci]].posY  = c_posY[ci]; \
//         CharKeyFrame[ci][CharFrameIndex[ci]].posZ  = c_posZ[ci]; \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rotY  = c_rotY[ci]; \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rTor  = c_rTor[ci]; \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rCab  = c_rCab[ci]; \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rPel  = c_rPel[ci]; \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rBD   = c_rBD[ci];  \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rAD   = c_rAD[ci];  \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rBI   = c_rBI[ci];  \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rAI   = c_rAI[ci];  \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rPD   = c_rPD[ci];  \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rTD   = c_rTD[ci];  \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rPieD = c_rPieD[ci]; \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rPI   = c_rPI[ci];  \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rTI   = c_rTI[ci];  \
//         CharKeyFrame[ci][CharFrameIndex[ci]].rPieI = c_rPieI[ci]; \
//         CharFrameIndex[ci]++; \
//     } while(0)

//     SAVE_KF(c);

//     for (int seg = 0; seg < ROUTE_SIZE - 1; seg++) {
//         float ax = route[seg].x, az = route[seg].z, aa = route[seg].angle;
//         float bx = route[seg + 1].x, bz = route[seg + 1].z, ba = route[seg +
//         1].angle;

//         float deltaAngle = ba - aa;
//         while (deltaAngle > 180.0f) deltaAngle -= 360.0f;
//         while (deltaAngle < -180.0f) deltaAngle += 360.0f;
//         float leanTor = deltaAngle * 0.08f;

//         for (int sub = 1; sub <= 4; sub++) {
//             float t = sub / 4.0f;
//             float mx = ax + (bx - ax) * t;
//             float mz = az + (bz - az) * t;
//             float ma = aa + (ba - aa) * t;

//             bool rightLead = ((seg * 4 + sub + (int)(phaseOffset * 2)) % 2 ==
//             0);

//             c_posX[c] = mx; c_posY[c] = WALK_Y; c_posZ[c] = mz; c_rotY[c] =
//             ma; c_rTor[c] = 3.0f; c_rCab[c] = 5.0f; c_rPel[c] = 2.0f +
//             leanTor * 0.3f;

//             if (rightLead) {
//                 c_rPD[c] = -25.0f;  c_rTD[c] = 10.0f;  c_rPieD[c] = -5.0f;
//                 c_rPI[c] = 20.0f;  c_rTI[c] = 5.0f;  c_rPieI[c] = 5.0f;
//                 c_rBI[c] = 12.0f + leanTor * 0.5f;  c_rAI[c] = 0.0f;
//                 c_rBD[c] = -12.0f - leanTor * 0.5f;  c_rAD[c] = 0.0f;
//             }
//             else {
//                 c_rPI[c] = -25.0f;  c_rTI[c] = 10.0f;  c_rPieI[c] = -5.0f;
//                 c_rPD[c] = 20.0f;  c_rTD[c] = 5.0f;  c_rPieD[c] = 5.0f;
//                 c_rBD[c] = 12.0f + leanTor * 0.5f;  c_rAD[c] = 0.0f;
//                 c_rBI[c] = -12.0f - leanTor * 0.5f;  c_rAI[c] = 0.0f;
//             }

//             SAVE_KF(c);
//         }
//     }

// #undef SAVE_KF

//     for (int i = 0; i < CharFrameIndex[c]; i++) {
//         while (CharKeyFrame[c][i].rotY > 180.0f) CharKeyFrame[c][i].rotY -=
//         360.0f; while (CharKeyFrame[c][i].rotY < -180.0f)
//         CharKeyFrame[c][i].rotY += 360.0f;
//     }

//     printf("[Char %d] %d keyframes generados (fase=%.1f)\n", c,
//     CharFrameIndex[c], phaseOffset);
// }

// void interpolation(int c) {
//     int p = charPlayIndex[c];

//     float dx = CharKeyFrame[c][p + 1].posX - CharKeyFrame[c][p].posX;
//     float dz = CharKeyFrame[c][p + 1].posZ - CharKeyFrame[c][p].posZ;
//     float dist = sqrtf(dx * dx + dz * dz);
//     int steps = (int)(dist * 400.0f);
//     if (steps < 200) steps = 200;
//     char_i_max_steps[c] = steps;

// #define LERP(field, inc_field) \
//         CharKeyFrame[c][p].inc_field = (CharKeyFrame[c][p+1].field -
//         CharKeyFrame[c][p].field) / steps

//     LERP(posX, incX);
//     LERP(posY, incY);
//     LERP(posZ, incZ);

//     float fromA = CharKeyFrame[c][p].rotY;
//     float toA = CharKeyFrame[c][p + 1].rotY;
//     float diff = toA - fromA;
//     while (diff > 180.0f) diff -= 360.0f;
//     while (diff < -180.0f) diff += 360.0f;
//     CharKeyFrame[c][p].incRotY = diff / steps;

//     LERP(rTor, iTor);
//     LERP(rCab, iCab);
//     LERP(rPel, iPel);
//     LERP(rBD, iBD);
//     LERP(rAD, iAD);
//     LERP(rBI, iBI);
//     LERP(rAI, iAI);
//     LERP(rPD, iPD);
//     LERP(rTD, iTD);
//     LERP(rPieD, iPieD);
//     LERP(rPI, iPI);
//     LERP(rTI, iTI);
//     LERP(rPieI, iPieI);
// #undef LERP
// }

// void resetChar(int c) {
//     c_posX[c] = CharKeyFrame[c][0].posX;
//     c_posY[c] = CharKeyFrame[c][0].posY;
//     c_posZ[c] = CharKeyFrame[c][0].posZ;
//     c_rotY[c] = CharKeyFrame[c][0].rotY;
//     c_rTor[c] = CharKeyFrame[c][0].rTor;
//     c_rCab[c] = CharKeyFrame[c][0].rCab;
//     c_rPel[c] = CharKeyFrame[c][0].rPel;
//     c_rBD[c] = CharKeyFrame[c][0].rBD;
//     c_rAD[c] = CharKeyFrame[c][0].rAD;
//     c_rBI[c] = CharKeyFrame[c][0].rBI;
//     c_rAI[c] = CharKeyFrame[c][0].rAI;
//     c_rPD[c] = CharKeyFrame[c][0].rPD;
//     c_rTD[c] = CharKeyFrame[c][0].rTD;
//     c_rPieD[c] = CharKeyFrame[c][0].rPieD;
//     c_rPI[c] = CharKeyFrame[c][0].rPI;
//     c_rTI[c] = CharKeyFrame[c][0].rTI;
//     c_rPieI[c] = CharKeyFrame[c][0].rPieI;
// }

// void CharAnimation() {
//     for (int c = 0; c < NUM_CHARS; c++) {
//         if (!charPlay[c]) continue;

//         if (char_i_curr_steps[c] >= char_i_max_steps[c]) {
//             charPlayIndex[c]++;
//             if (charPlayIndex[c] > CharFrameIndex[c] - 2) {
//                 charPlayIndex[c] = 0;
//                 char_i_curr_steps[c] = 0;
//                 resetChar(c);
//                 interpolation(c);
//             }
//             else {
//                 char_i_curr_steps[c] = 0;
//                 interpolation(c);
//             }
//         }
//         else {
//             int p = charPlayIndex[c];
//             c_posX[c] += CharKeyFrame[c][p].incX;
//             c_posY[c] += CharKeyFrame[c][p].incY;
//             c_posZ[c] += CharKeyFrame[c][p].incZ;
//             c_rotY[c] += CharKeyFrame[c][p].incRotY;
//             while (c_rotY[c] > 180.0f) c_rotY[c] -= 360.0f;
//             while (c_rotY[c] < -180.0f) c_rotY[c] += 360.0f;
//             c_rTor[c] += CharKeyFrame[c][p].iTor;
//             c_rCab[c] += CharKeyFrame[c][p].iCab;
//             c_rPel[c] += CharKeyFrame[c][p].iPel;
//             c_rBD[c] += CharKeyFrame[c][p].iBD;
//             c_rAD[c] += CharKeyFrame[c][p].iAD;
//             c_rBI[c] += CharKeyFrame[c][p].iBI;
//             c_rAI[c] += CharKeyFrame[c][p].iAI;
//             c_rPD[c] += CharKeyFrame[c][p].iPD;
//             c_rTD[c] += CharKeyFrame[c][p].iTD;
//             c_rPieD[c] += CharKeyFrame[c][p].iPieD;
//             c_rPI[c] += CharKeyFrame[c][p].iPI;
//             c_rTI[c] += CharKeyFrame[c][p].iTI;
//             c_rPieI[c] += CharKeyFrame[c][p].iPieI;
//             char_i_curr_steps[c]++;
//         }
//     }
// }

// //
// ---------------------------------------------------------------------------
// // DrawCharacter
// // Dibuja al personaje articulado (elder_Female) en su posicion actual.
// //
// ---------------------------------------------------------------------------
// void DrawCharacter(int c, Shader& shader, GLint mLoc,
//     Model& Torso, Model& Pelvis, Model& Cabeza,
//     Model& TB_D, Model& TB_I,
//     Model& P_D, Model& T_D, Model& Pie_D,
//     Model& P_I, Model& T_I, Model& Pie_I)
// {
//     glm::mat4 mG = glm::translate(glm::mat4(1.0f),
//         glm::vec3(c_posX[c], c_posY[c], c_posZ[c]));
//     mG = glm::rotate(mG, glm::radians(c_rotY[c]), glm::vec3(0, 1, 0));
//     mG = glm::scale(mG, glm::vec3(0.65f));

//     // --- Torso ---
//     glm::mat4 mT = glm::rotate(mG, glm::radians(c_rTor[c]), glm::vec3(1, 0,
//     0)); mT = glm::translate(mT, glm::vec3(0.0f, 0.2f, 0.0f));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mT));
//     Torso.Draw(shader);

//     // --- Cabeza ---
//     glm::mat4 mC = glm::translate(mT, glm::vec3(0.0f, 0.44f, 0.0f));
//     mC = glm::rotate(mC, glm::radians(c_rCab[c]), glm::vec3(1, 0, 0));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mC));
//     Cabeza.Draw(shader);

//     // ================= BRAZO DERECHO =================
//     glm::mat4 mBD = glm::translate(mT, glm::vec3(-0.10f, -0.55f, -0.08f));
//     mBD = glm::rotate(mBD, glm::radians(42.0f), glm::vec3(0, 1, 1));
//     mBD = glm::translate(mBD, glm::vec3(0.0f, 0.0f, 0.2f));
//     mBD = glm::translate(mBD, glm::vec3(0.50f, 0.0f, 0.0f));
//     mBD = glm::rotate(mBD, glm::radians(-90.0f), glm::vec3(0, 0, 1));
//     mBD = glm::rotate(mBD, glm::radians(c_rBD[c]), glm::vec3(1, 0, 0));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mBD));
//     TB_D.Draw(shader);

//     // ================= BRAZO IZQUIERDO =================
//     glm::mat4 mBI = glm::translate(mT, glm::vec3(0.10f, -0.50f, 0.1f));
//     mBI = glm::rotate(mBI, glm::radians(-42.0f), glm::vec3(0, 1, 1));
//     mBI = glm::translate(mBI, glm::vec3(-0.50f, 0.0f, 0.0f));
//     mBI = glm::rotate(mBI, glm::radians(90.0f), glm::vec3(0, 0, 1));
//     mBI = glm::rotate(mBI, glm::radians(c_rBI[c]), glm::vec3(1, 0, 0));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mBI));
//     TB_I.Draw(shader);

//     // --- Pelvis ---
//     glm::mat4 mPel = glm::translate(mG, glm::vec3(0.0f, 0.0f, 0.0f));
//     mPel = glm::rotate(mPel, glm::radians(c_rPel[c]), glm::vec3(0, 1, 0));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPel));
//     Pelvis.Draw(shader);

//     // --- Pierna Derecha ---
//     glm::mat4 mPD = glm::translate(mPel, glm::vec3(-0.15f, -0.46f, 0.0f));
//     mPD = glm::rotate(mPD, glm::radians(c_rPD[c]), glm::vec3(1, 0, 0));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPD));
//     P_D.Draw(shader);

//     glm::mat4 mTD = glm::translate(mPD, glm::vec3(0.0f, -0.27f, 0.0f));
//     mTD = glm::rotate(mTD, glm::radians(c_rTD[c]), glm::vec3(1, 0, 0));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mTD));
//     T_D.Draw(shader);

//     glm::mat4 mPieD = glm::translate(mTD, glm::vec3(0.0f, -0.49f, 0.05f));
//     mPieD = glm::rotate(mPieD, glm::radians(c_rPieD[c]), glm::vec3(1, 0, 0));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPieD));
//     Pie_D.Draw(shader);

//     // --- Pierna Izquierda ---
//     glm::mat4 mPI = glm::translate(mPel, glm::vec3(0.15f, -0.46f, 0.0f));
//     mPI = glm::rotate(mPI, glm::radians(c_rPI[c]), glm::vec3(1, 0, 0));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPI));
//     P_I.Draw(shader);

//     glm::mat4 mTI = glm::translate(mPI, glm::vec3(0.0f, -0.27f, 0.0f));
//     mTI = glm::rotate(mTI, glm::radians(c_rTI[c]), glm::vec3(1, 0, 0));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mTI));
//     T_I.Draw(shader);

//     glm::mat4 mPieI = glm::translate(mTI, glm::vec3(0.0f, -0.49f, 0.05f));
//     mPieI = glm::rotate(mPieI, glm::radians(c_rPieI[c]), glm::vec3(1, 0, 0));
//     glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(mPieI));
//     Pie_I.Draw(shader);
// }

// //
// =============================================================================
// // VERTICES PARA FIGURAS BASICAS (cubo)
// //
// =============================================================================
// float vertices[] = {
//     -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.5f,  -0.5f, -0.5f,
//     0.0f,  0.0f,  -1.0f, 0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
//     0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, -0.5f, 0.5f,  -0.5f,
//     0.0f,  0.0f,  -1.0f, -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,

//     -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,
//     0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
//     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  -0.5f, 0.5f,  0.5f,
//     0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,

//     -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f,
//     -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
//     -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, 0.5f,
//     -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,

//     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
//     1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
//     0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, 0.5f,
//     1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

//     -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, -0.5f,
//     0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
//     0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, 0.5f,
//     0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,

//     -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
//     0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
//     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,
//     0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f
// };

// GLfloat skyboxVertices[] = {
//     -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
//     1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

//     -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
//     -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

//     1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
//     1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

//     -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
//     1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

//     -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
//     1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

//     -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
//     1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
// };

// //
// =============================================================================
// // MAIN
// //
// =============================================================================
// int main() {
//     // Inicializar memoria de keyframes del personaje y construir el ciclo
//     // de caminata proceduralmente (sin archivos, sin terminal).
//     for (int c = 0; c < NUM_CHARS; c++)
//         for (int i = 0; i < CHAR_MAX_FRAMES; i++)
//             memset(&CharKeyFrame[c][i], 0, sizeof(CHAR_FRAME));

//     for (int c = 0; c < NUM_CHARS; c++) char_i_max_steps[c] = 1500;
//     BuildWalkCycle(0, 0.0f);

//     for (int c = 0; c < NUM_CHARS; c++) {
//         if (CharFrameIndex[c] > 1) {
//             resetChar(c);
//             charPlayIndex[c] = 0;
//             char_i_curr_steps[c] = 0;
//             interpolation(c);
//             charPlay[c] = true;
//         }
//     }

//     // ---------- GLFW ----------
//     glfwInit();
//     GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
//         "Proyecto Final CGIHC - Integrado", nullptr, nullptr);
//     if (nullptr == window) {
//         std::cout << "Failed to create GLFW window" << std::endl;
//         glfwTerminate();
//         return EXIT_FAILURE;
//     }
//     glfwMakeContextCurrent(window);
//     glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
//     glfwSetKeyCallback(window, KeyCallback);
//     glfwSetCursorPosCallback(window, MouseCallback);
//     glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

//     glewExperimental = GL_TRUE;
//     glewInit();
//     glGetError();

//     glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
//     glEnable(GL_DEPTH_TEST);

//     // ---------- Shaders ----------
//     Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
//     Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");
//     Shader skyboxShader("Shader/SkyBox.vs", "Shader/SkyBox.frag");

//     // ---------- Modelos de la escena ----------
//     Model Escena((char*)"Models/trolleo.obj");
//     Model Stands((char*)"Models/stands.obj");
//     Model Lampara1((char*)"Models/lampara1.obj");
//     Model Lampara2((char*)"Models/lampara2.obj");

//     // ---------- Modelos del helicoptero ----------
//     Model HelicCuerpo((char*)"Models/helicopteroCuerpo.obj");
//     Model HelicHelice((char*)"Models/helicopteroHelice.obj");
//     Model HelicCola  ((char*)"Models/helicopteroCola.obj");

//     // ---------- Modelos del personaje ----------
//     Model Torso ((char*)"Models/mujer1/elder_Female_torso.obj");
//     Model Pelvis((char*)"Models/mujer1/elder_Female_pelvis.obj");
//     Model Cabeza((char*)"Models/mujer1/elder_Female_cabeza.obj");
//     Model TB_D  ((char*)"Models/mujer1/elder_Female_tbrazo_d.obj");
//     Model TB_I  ((char*)"Models/mujer1/elder_Female_tbrazo_i.obj");
//     Model P_D   ((char*)"Models/mujer1/elder_Female_pierna_d.obj");
//     Model T_D   ((char*)"Models/mujer1/elder_Female_tobillo_d.obj");
//     Model Pie_D ((char*)"Models/mujer1/elder_Female_pie_d.obj");
//     Model P_I   ((char*)"Models/mujer1/elder_Female_pierna_i.obj");
//     Model T_I   ((char*)"Models/mujer1/elder_Female_tobillo_i.obj");
//     Model Pie_I ((char*)"Models/mujer1/elder_Female_pie_i.obj");

//     // ---------- VAO/VBO para cubo (usado por grua/brazo procedural)
//     ---------- GLuint indices[] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
//     10, 11,
//                         12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
//                         24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35 };

//     GLuint VBO, VAO, EBO;
//     glGenVertexArrays(1, &VAO);
//     glGenBuffers(1, &VBO);
//     glGenBuffers(1, &EBO);

//     glBindVertexArray(VAO);
//     glBindBuffer(GL_ARRAY_BUFFER, VBO);
//     glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
//     GL_STATIC_DRAW);

//     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
//     glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
//     GL_STATIC_DRAW);

//     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
//     (GLvoid*)0); glEnableVertexAttribArray(0); glVertexAttribPointer(1, 3,
//     GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
//     glEnableVertexAttribArray(1);

//     // ---------- Skybox ----------
//     GLuint skyboxVAO, skyboxVBO;
//     glGenVertexArrays(1, &skyboxVAO);
//     glGenBuffers(1, &skyboxVBO);
//     glBindVertexArray(skyboxVAO);
//     glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
//     glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
//     GL_STATIC_DRAW); glEnableVertexAttribArray(0); glVertexAttribPointer(0,
//     3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
//     glBindVertexArray(0);

//     vector<const GLchar*> faces;
//     faces.push_back("SkyBox/right.jpg");
//     faces.push_back("SkyBox/left.jpg");
//     faces.push_back("SkyBox/top.jpg");
//     faces.push_back("SkyBox/bottom.jpg");
//     faces.push_back("SkyBox/back.jpg");
//     faces.push_back("SkyBox/front.jpg");
//     GLuint cubemapTexture = TextureLoading::LoadCubemap(faces);

//     lightingShader.Use();
//     glUniform1i(glGetUniformLocation(lightingShader.Program,
//     "Material.difuse"), 0);
//     glUniform1i(glGetUniformLocation(lightingShader.Program,
//     "Material.specular"), 1);

//     glm::mat4 projection = glm::perspective(camera.GetZoom(),
//         (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

//     // ---------- Lambdas de dibujo ----------
//     auto DibujarGeometria = [&](glm::mat4 mBase, glm::vec3 escala, glm::vec3
//     color, GLint modelLoc) {
//         glm::mat4 mForma = glm::scale(mBase, escala);
//         glUniform3fv(glGetUniformLocation(lightingShader.Program,
//         "objectColor"), 1, glm::value_ptr(color));
//         glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mForma));
//         glDrawArrays(GL_TRIANGLES, 0, 36);
//         };

//     auto DibujarDedoDetallado = [&](glm::mat4 baseDedo, float flexion, float
//     offsetX, float offsetY, float scaleGrosor, float scaleLargo, GLint
//     modelLoc) {
//         glm::mat4 mDedo = baseDedo;
//         glm::vec3 colorNudillo(0.15f, 0.15f, 0.15f);
//         glm::vec3 colorFalange(0.8f, 0.8f, 0.8f);

//         mDedo = glm::translate(mDedo, glm::vec3(offsetX, offsetY, 0.0f));
//         mDedo = glm::rotate(mDedo, glm::radians(flexion), glm::vec3(1.0f,
//         0.0f, 0.0f)); DibujarGeometria(mDedo, glm::vec3(scaleGrosor * 1.2f),
//         colorNudillo, modelLoc);

//         glm::mat4 mRender1 = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo
//         / 2, 0.0f)); DibujarGeometria(mRender1, glm::vec3(scaleGrosor,
//         scaleLargo, scaleGrosor), colorFalange, modelLoc);

//         mDedo = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo, 0.0f));
//         mDedo = glm::rotate(mDedo, glm::radians(flexion * 0.8f),
//         glm::vec3(1.0f, 0.0f, 0.0f)); DibujarGeometria(mDedo,
//         glm::vec3(scaleGrosor * 1.1f), colorNudillo, modelLoc);

//         glm::mat4 mRender2 = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo
//         * 0.45f, 0.0f)); DibujarGeometria(mRender2, glm::vec3(scaleGrosor *
//         0.9f, scaleLargo * 0.9f, scaleGrosor * 0.9f), colorFalange,
//         modelLoc);

//         mDedo = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo * 0.9f,
//         0.0f)); mDedo = glm::rotate(mDedo, glm::radians(flexion * 0.6f),
//         glm::vec3(1.0f, 0.0f, 0.0f)); DibujarGeometria(mDedo,
//         glm::vec3(scaleGrosor * 0.9f), colorNudillo, modelLoc);

//         glm::mat4 mRender3 = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo
//         * 0.35f, 0.0f)); DibujarGeometria(mRender3, glm::vec3(scaleGrosor *
//         0.8f, scaleLargo * 0.7f, scaleGrosor * 0.8f), colorFalange,
//         modelLoc);
//         };

//     // --- Carga de animacion combinada de grua + brazo ---
//     loadFromFile();

//     //
//     =========================================================================
//     // BUCLE PRINCIPAL
//     //
//     =========================================================================
//     while (!glfwWindowShouldClose(window)) {
//         GLfloat currentFrame = (GLfloat)glfwGetTime();
//         deltaTime = currentFrame - lastFrame;
//         lastFrame = currentFrame;

//         glfwPollEvents();
//         DoMovement();

//         // Animaciones controladas por switches independientes
//         if (gruaBrazoAnimOn) {
//             Animation();        // grua + brazo
//         }
//         if (personajeAnimOn) {
//             CharAnimation();    // personaje
//         }
//         if (helicopteroAnimOn) {
//             helicopteroHover += helicopteroHoverDir * deltaTime * 0.4f;
//             if (helicopteroHover >  0.5f) helicopteroHoverDir = -1.0f;
//             if (helicopteroHover < -0.5f) helicopteroHoverDir =  1.0f;
//             rotHelice += deltaTime * 600.0f;
//             if (rotHelice >= 360.0f) rotHelice -= 360.0f;
//             rotCola   += deltaTime * 600.0f;
//             if (rotCola   >= 360.0f) rotCola   -= 360.0f;
//         }

//         glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//         glEnable(GL_DEPTH_TEST);

//         lightingShader.Use();

//         GLint viewPosLoc = glGetUniformLocation(lightingShader.Program,
//         "viewPos"); glUniform3f(viewPosLoc, camera.GetPosition().x,
//         camera.GetPosition().y, camera.GetPosition().z);

//         glUniform3f(glGetUniformLocation(lightingShader.Program,
//         "dirLight.direction"), 0.0f, -1.0f, 0.0f);
//         glUniform3f(glGetUniformLocation(lightingShader.Program,
//         "dirLight.ambient"), 0.0f, 0.0f, 0.0f);
//         glUniform3f(glGetUniformLocation(lightingShader.Program,
//         "dirLight.diffuse"), 0.0f, 0.0f, 0.0f);
//         glUniform3f(glGetUniformLocation(lightingShader.Program,
//         "dirLight.specular"), 0.0f, 0.0f, 0.0f);

//         // 11 lamparas point lights
//         float lA = lampsOn ? 0.05f : 0.0f;
//         float lD = lampsOn ? 0.90f : 0.0f;
//         float lS = lampsOn ? 0.50f : 0.0f;
//         glm::vec3 allLampPos[11];
//         for (int i = 0; i < 6; i++) allLampPos[i] = posLamp1[i];
//         for (int i = 0; i < 5; i++) allLampPos[6 + i] = posLamp2[i];

//         char buf[64];
//         for (int i = 0; i < 11; i++) {
//             snprintf(buf, sizeof(buf), "pointLights[%d].position", i);
//             glUniform3f(glGetUniformLocation(lightingShader.Program, buf),
//             allLampPos[i].x, allLampPos[i].y - 0.5f, allLampPos[i].z);
//             snprintf(buf, sizeof(buf), "pointLights[%d].ambient", i);
//             glUniform3f(glGetUniformLocation(lightingShader.Program, buf),
//             lA, lA, lA); snprintf(buf, sizeof(buf),
//             "pointLights[%d].diffuse", i);
//             glUniform3f(glGetUniformLocation(lightingShader.Program, buf),
//             lD, lD * 0.95f, lD * 0.8f); snprintf(buf, sizeof(buf),
//             "pointLights[%d].specular", i);
//             glUniform3f(glGetUniformLocation(lightingShader.Program, buf),
//             lS, lS, lS); snprintf(buf, sizeof(buf),
//             "pointLights[%d].constant", i);
//             glUniform1f(glGetUniformLocation(lightingShader.Program,
//             buf), 1.0f); snprintf(buf, sizeof(buf), "pointLights[%d].linear",
//             i); glUniform1f(glGetUniformLocation(lightingShader.Program,
//             buf), 0.14f); snprintf(buf, sizeof(buf),
//             "pointLights[%d].quadratic", i);
//             glUniform1f(glGetUniformLocation(lightingShader.Program, buf),
//             0.07f);
//         }

//         // Spotlight de la camara
//         glUniform3f(glGetUniformLocation(lightingShader.Program,
//         "spotLight.position"), camera.GetPosition().x,
//         camera.GetPosition().y, camera.GetPosition().z);
//         glUniform3f(glGetUniformLocation(lightingShader.Program,
//         "spotLight.direction"), camera.GetFront().x, camera.GetFront().y,
//         camera.GetFront().z);
//         glUniform3f(glGetUniformLocation(lightingShader.Program,
//         "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
//         glUniform3f(glGetUniformLocation(lightingShader.Program,
//         "spotLight.diffuse"), 1.0f, 1.0f, 1.0f);
//         glUniform3f(glGetUniformLocation(lightingShader.Program,
//         "spotLight.specular"), 1.0f, 1.0f, 1.0f);
//         glUniform1f(glGetUniformLocation(lightingShader.Program,
//         "spotLight.constant"), 1.0f);
//         glUniform1f(glGetUniformLocation(lightingShader.Program,
//         "spotLight.linear"), 0.09f);
//         glUniform1f(glGetUniformLocation(lightingShader.Program,
//         "spotLight.quadratic"), 0.032f);
//         glUniform1f(glGetUniformLocation(lightingShader.Program,
//         "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
//         glUniform1f(glGetUniformLocation(lightingShader.Program,
//         "spotLight.outerCutOff"), glm::cos(glm::radians(15.0f)));

//         glUniform1f(glGetUniformLocation(lightingShader.Program,
//         "material.shininess"), 32.0f);

//         glm::mat4 view = camera.GetViewMatrix();
//         GLint modelLoc = glGetUniformLocation(lightingShader.Program,
//         "model"); GLint viewLoc =
//         glGetUniformLocation(lightingShader.Program, "view"); GLint projLoc =
//         glGetUniformLocation(lightingShader.Program, "projection");

//         glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
//         glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

//         glm::mat4 model(1);

//         //
//         =====================================================================
//         // ESCENA Y STANDS (CON TEXTURAS)
//         //
//         =====================================================================
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "useTexture"), 1);

//         // Escena - doble pase para transparencia
//         glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
//         glDepthMask(GL_TRUE); glDisable(GL_BLEND);
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "transparency"), 1); Escena.Draw(lightingShader);

//         glDepthMask(GL_FALSE); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,
//         GL_ONE_MINUS_SRC_ALPHA);
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "transparency"), 2); Escena.Draw(lightingShader);

//         glDepthMask(GL_TRUE); glDisable(GL_BLEND);
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "transparency"), 0);

//         // Stands - doble pase para transparencia
//         glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
//         glDepthMask(GL_TRUE); glDisable(GL_BLEND);
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "transparency"), 1); Stands.Draw(lightingShader);

//         glDepthMask(GL_FALSE); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,
//         GL_ONE_MINUS_SRC_ALPHA);
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "transparency"), 2); Stands.Draw(lightingShader);

//         glDepthMask(GL_TRUE); glDisable(GL_BLEND);
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "transparency"), 0);

//         //
//         =====================================================================
//         // PERSONAJE (CON TEXTURAS - mismas opciones que la escena)
//         //
//         =====================================================================
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "useTexture"), 1); for (int c = 0; c < NUM_CHARS; c++) {
//             DrawCharacter(c, lightingShader, modelLoc,
//                 Torso, Pelvis, Cabeza,
//                 TB_D, TB_I,
//                 P_D, T_D, Pie_D,
//                 P_I, T_I, Pie_I);
//         }

//         //
//         =====================================================================
//         // HELICOPTERO (CON TEXTURAS, JERARQUICO)
//         //
//         =====================================================================
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "useTexture"), 1);
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "transparency"), 0);

//         // Cuerpo (padre) — posicion world + hover + rotacion Blender
//         Z→OpenGL Y glm::vec3 helicopteroBase(5.9861f, 0.8229f +
//         helicopteroHover, 8.8815f); glm::mat4 mHelicCuerpo =
//         glm::translate(glm::mat4(1.0f), helicopteroBase); mHelicCuerpo =
//         glm::rotate(mHelicCuerpo, glm::radians(229.68f),
//         glm::vec3(0.0f, 1.0f, 0.0f)); glUniformMatrix4fv(modelLoc, 1,
//         GL_FALSE, glm::value_ptr(mHelicCuerpo));
//         HelicCuerpo.Draw(lightingShader);

//         // Helice principal (hijo del cuerpo) — gira en Y
//         glm::vec3 heliceOffset(-0.0292f, 0.1451f, -0.0248f);
//         glm::mat4 mHelicHelice = glm::translate(mHelicCuerpo, heliceOffset);
//         mHelicHelice = glm::rotate(mHelicHelice, glm::radians(rotHelice),
//         glm::vec3(0.0f, 1.0f, 0.0f)); glUniformMatrix4fv(modelLoc, 1,
//         GL_FALSE, glm::value_ptr(mHelicHelice));
//         HelicHelice.Draw(lightingShader);

//         // Helice de cola (hijo del cuerpo) — gira en X
//         glm::vec3 colaOffset(0.3201f, 0.1775f, 0.2737f);
//         glm::mat4 mHelicCola = glm::translate(mHelicCuerpo, colaOffset);
//         mHelicCola = glm::rotate(mHelicCola, glm::radians(rotCola),
//         glm::vec3(1.0f, 0.0f, 0.0f)); glUniformMatrix4fv(modelLoc, 1,
//         GL_FALSE, glm::value_ptr(mHelicCola));
//         HelicCola.Draw(lightingShader);

//         //
//         =====================================================================
//         // GRUA TORRE + BRAZO ROBOTICO (PROCEDURALES, SIN TEXTURAS)
//         //
//         =====================================================================
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "useTexture"), 0);

//         glBindVertexArray(VAO);
//         std::stack<glm::mat4> pila;
//         glm::vec3 colorGrua;

//         // ---> GRUA TORRE <---
//         glm::mat4 modelBaseGrua = glm::translate(glm::mat4(1.0f),
//         glm::vec3(-8.5f, 0.7f, -9.3f)); modelBaseGrua =
//         glm::scale(modelBaseGrua, glm::vec3(0.125f)); modelBaseGrua =
//         glm::rotate(modelBaseGrua, glm::radians(90.0f), glm::vec3(0.0f, 1.0f,
//         0.0f));

//         colorGrua = glm::vec3(0.3f, 0.3f, 0.3f);
//         DibujarGeometria(modelBaseGrua, glm::vec3(3.0f, 0.5f, 3.0f),
//         colorGrua, modelLoc);

//         int numSegmentos = 6;
//         float altoSeg = 1.5f, anchoSeg = 1.2f, grosor = 0.1f;
//         colorGrua = glm::vec3(0.9f, 0.7f, 0.1f);

//         for (int i = 0; i < numSegmentos; i++) {
//             glm::mat4 baseSeg = glm::translate(modelBaseGrua, glm::vec3(0.0f,
//             0.25f + (i * altoSeg), 0.0f)); for (int x : {-1, 1}) {
//                 for (int z : {-1, 1}) {
//                     glm::mat4 poste = glm::translate(baseSeg, glm::vec3(x *
//                     anchoSeg / 2, altoSeg / 2, z * anchoSeg / 2));
//                     DibujarGeometria(poste, glm::vec3(grosor, altoSeg,
//                     grosor), colorGrua, modelLoc);
//                 }
//             }
//             for (int z : {-1, 1}) {
//                 glm::mat4 horiz = glm::translate(baseSeg, glm::vec3(0.0f,
//                 altoSeg, z * anchoSeg / 2)); DibujarGeometria(horiz,
//                 glm::vec3(anchoSeg + grosor, grosor, grosor), colorGrua,
//                 modelLoc);
//             }
//             for (int x : {-1, 1}) {
//                 glm::mat4 horiz = glm::translate(baseSeg, glm::vec3(x *
//                 anchoSeg / 2, altoSeg, 0.0f)); DibujarGeometria(horiz,
//                 glm::vec3(grosor, grosor, anchoSeg + grosor), colorGrua,
//                 modelLoc);
//             }
//             float anguloDiag = atan2(altoSeg, anchoSeg);
//             float largoDiag = sqrt(altoSeg * altoSeg + anchoSeg * anchoSeg);
//             for (int z : {-1, 1}) {
//                 glm::mat4 diag = glm::translate(baseSeg, glm::vec3(0.0f,
//                 altoSeg / 2, z * anchoSeg / 2)); diag = glm::rotate(diag, (z
//                 == 1) ? anguloDiag : -anguloDiag, glm::vec3(0.0f,
//                 0.0f, 1.0f)); DibujarGeometria(diag, glm::vec3(largoDiag,
//                 grosor / 2, grosor / 2), colorGrua, modelLoc);
//             }
//         }

//         float alturaTorre = 0.25f + (numSegmentos * altoSeg);
//         glm::mat4 modelTorreta = glm::translate(modelBaseGrua,
//         glm::vec3(0.0f, alturaTorre + 0.2f, 0.0f)); modelTorreta =
//         glm::rotate(modelTorreta, glm::radians(rotTorreta),
//         glm::vec3(0.0f, 1.0f, 0.0f)); pila.push(modelTorreta);

//         DibujarGeometria(modelTorreta, glm::vec3(2.0f, 0.4f, 2.0f),
//         glm::vec3(0.2f), modelLoc);

//         glm::mat4 renderCabina = glm::translate(modelTorreta,
//         glm::vec3(-0.9f, -0.6f, 0.9f)); DibujarGeometria(renderCabina,
//         glm::vec3(0.8f, 1.5f, 1.0f), glm::vec3(0.1f, 0.4f, 0.7f), modelLoc);

//         glm::mat4 renderTrasero = glm::translate(modelTorreta,
//         glm::vec3(0.0f, 0.3f, -1.2f)); DibujarGeometria(renderTrasero,
//         glm::vec3(1.5f, 0.8f, 1.5f), glm::vec3(0.15f), modelLoc);

//         glm::mat4 modelMastil = glm::translate(modelTorreta,
//         glm::vec3(0.0f, 1.5f, 0.0f)); pila.push(modelMastil);
//         DibujarGeometria(modelMastil, glm::vec3(0.8f, 3.0f, 0.8f), colorGrua,
//         modelLoc);

//         glm::mat4 modelPlumaBase = pila.top();
//         modelPlumaBase = glm::translate(modelPlumaBase, glm::vec3(0.0f, 0.0f,
//         0.5f)); modelPlumaBase = glm::rotate(modelPlumaBase,
//         glm::radians(elevacionPluma), glm::vec3(1.0f, 0.0f, 0.0f));
//         pila.push(modelPlumaBase);

//         glm::mat4 renderPluma = glm::translate(modelPlumaBase,
//         glm::vec3(0.0f, 0.0f, 4.0f)); DibujarGeometria(renderPluma,
//         glm::vec3(0.8f, 0.8f, 8.0f), colorGrua, modelLoc);

//         glm::mat4 modelTeleNode = pila.top();
//         modelTeleNode = glm::translate(modelTeleNode, glm::vec3(0.0f,
//         0.0f, 1.0f + extensionTelescopio)); pila.push(modelTeleNode);

//         glm::mat4 renderTele = glm::translate(modelTeleNode, glm::vec3(0.0f,
//         0.0f, 4.0f)); DibujarGeometria(renderTele, glm::vec3(0.6f,
//         0.6f, 8.0f), glm::vec3(0.7f), modelLoc);

//         glm::mat4 modelAnclajeGiro = glm::translate(modelTeleNode,
//         glm::vec3(0.0f, -0.3f, 8.0f)); pila.pop(); pila.pop(); pila.pop();
//         pila.pop();

//         modelAnclajeGiro = glm::rotate(modelAnclajeGiro,
//         glm::radians(-elevacionPluma), glm::vec3(1.0f, 0.0f, 0.0f));
//         modelAnclajeGiro = glm::rotate(modelAnclajeGiro,
//         glm::radians(anguloPenduloX), glm::vec3(1.0f, 0.0f, 0.0f));
//         modelAnclajeGiro = glm::rotate(modelAnclajeGiro,
//         glm::radians(anguloPenduloZ), glm::vec3(0.0f, 0.0f, 1.0f));

//         float conWidth = 3.6f, conHeight = 1.2f, conDepth = 1.2f;
//         float spreadX = conWidth / 2.0f;

//         DibujarGeometria(modelAnclajeGiro, glm::vec3(0.4f), glm::vec3(0.1f),
//         modelLoc);

//         float diagLength = sqrt((spreadX * spreadX) + (largoCableActual *
//         largoCableActual)); float angleCable = atan2(spreadX,
//         largoCableActual);

//         for (int i = -1; i <= 1; i += 2) {
//             float xDir = (float)i;
//             glm::mat4 renderCable = glm::translate(modelAnclajeGiro,
//             glm::vec3(xDir * spreadX / 2.0f, -largoCableActual / 2.0f,
//             0.0f)); renderCable = glm::rotate(renderCable, xDir * angleCable,
//             glm::vec3(0.0f, 0.0f, 1.0f)); DibujarGeometria(renderCable,
//             glm::vec3(0.04f, diagLength, 0.04f), glm::vec3(0.0f), modelLoc);
//         }

//         glm::mat4 modelContenedorBase = glm::translate(modelAnclajeGiro,
//         glm::vec3(0.0f, -largoCableActual - 0.6f, 0.0f));
//         DibujarGeometria(modelContenedorBase, glm::vec3(3.6f, 1.2f, 1.2f),
//         glm::vec3(0.5f, 0.15f, 0.1f), modelLoc);

//         int numRibs = 14;
//         float ribGrosor = 0.06f, ribSaliente = 0.05f;
//         glm::vec3 colorRib(0.4f, 0.1f, 0.05f);

//         for (int i = 0; i < numRibs; i++) {
//             float xPosRib = -conWidth / 2.0f + (i * conWidth / (numRibs -
//             1)); glm::mat4 renderRibFrente =
//             glm::translate(modelContenedorBase, glm::vec3(xPosRib, 0.0f,
//             conDepth / 2.0f + ribSaliente / 2.0f));
//             DibujarGeometria(renderRibFrente, glm::vec3(ribGrosor, conHeight
//             + 0.01f, ribSaliente), colorRib, modelLoc);

//             glm::mat4 renderRibAtras = glm::translate(modelContenedorBase,
//             glm::vec3(xPosRib, 0.0f, -conDepth / 2.0f - ribSaliente / 2.0f));
//             DibujarGeometria(renderRibAtras, glm::vec3(ribGrosor, conHeight +
//             0.01f, ribSaliente), colorRib, modelLoc);
//         }

//         // ---> BRAZO ROBOTICO Y PIEZA OBJETIVO <---
//         glm::mat4 mPieza = glm::mat4(1.0f);
//         mPieza = glm::translate(mPieza, glm::vec3(-6.5f, -4.5f, -3.0f));
//         mPieza = glm::scale(mPieza, glm::vec3(0.125f));
//         DibujarGeometria(mPieza, glm::vec3(1.2f), glm::vec3(0.9f, 0.2f,
//         0.2f), modelLoc);

//         glm::mat4 modelBaseBrazo = glm::translate(glm::mat4(1.0f),
//         glm::vec3(-8.3f, 0.75f, -4.0f)); modelBaseBrazo =
//         glm::scale(modelBaseBrazo, glm::vec3(0.125f));

//         glm::vec3 colorBrazo(0.6f, 0.6f, 0.65f);
//         glm::vec3 colorArticulacion(0.15f, 0.15f, 0.15f);

//         modelBaseBrazo = glm::rotate(modelBaseBrazo, glm::radians(baseY),
//         glm::vec3(0.0f, 1.0f, 0.0f)); pila.push(modelBaseBrazo);
//         DibujarGeometria(modelBaseBrazo, glm::vec3(3.0f, 0.5f, 3.0f),
//         colorArticulacion, modelLoc);

//         glm::mat4 modHombro = glm::translate(modelBaseBrazo,
//         glm::vec3(0.0f, 1.0f, 0.0f)); modHombro = glm::rotate(modHombro,
//         glm::radians(hombroX), glm::vec3(1.0f, 0.0f, 0.0f)); modHombro =
//         glm::rotate(modHombro, glm::radians(hombroZ), glm::vec3(0.0f,
//         0.0f, 1.0f)); pila.push(modHombro); DibujarGeometria(modHombro,
//         glm::vec3(1.5f), colorArticulacion, modelLoc);

//         glm::mat4 renderBrazo = glm::translate(modHombro,
//         glm::vec3(0.0f, 2.0f, 0.0f)); DibujarGeometria(renderBrazo,
//         glm::vec3(1.2f, 4.0f, 1.2f), colorBrazo, modelLoc);

//         glm::mat4 modCodo = pila.top();
//         modCodo = glm::translate(modCodo, glm::vec3(0.0f, 4.0f, 0.0f));
//         modCodo = glm::rotate(modCodo, glm::radians(codo), glm::vec3(1.0f,
//         0.0f, 0.0f)); pila.push(modCodo); DibujarGeometria(modCodo,
//         glm::vec3(1.4f, 1.2f, 1.4f), colorArticulacion, modelLoc);

//         glm::mat4 barraIzq = glm::translate(modCodo, glm::vec3(-0.4f, 2.0f,
//         0.0f)); DibujarGeometria(barraIzq, glm::vec3(0.3f, 4.0f, 0.8f),
//         colorBrazo, modelLoc);

//         glm::mat4 barraDer = glm::translate(modCodo, glm::vec3(0.4f, 2.0f,
//         0.0f)); DibujarGeometria(barraDer, glm::vec3(0.3f, 4.0f, 0.8f),
//         colorBrazo, modelLoc);

//         glm::mat4 piston = glm::translate(modCodo, glm::vec3(0.0f, 2.0f,
//         0.0f)); DibujarGeometria(piston, glm::vec3(0.2f, 3.8f, 0.2f),
//         glm::vec3(0.2f), modelLoc);

//         glm::mat4 modMuneca = pila.top();
//         modMuneca = glm::translate(modMuneca, glm::vec3(0.0f, 4.0f, 0.0f));
//         modMuneca = glm::rotate(modMuneca, glm::radians(munecaX),
//         glm::vec3(1.0f, 0.0f, 0.0f)); modMuneca = glm::rotate(modMuneca,
//         glm::radians(munecaZ), glm::vec3(0.0f, 0.0f, 1.0f));
//         pila.push(modMuneca);
//         DibujarGeometria(modMuneca, glm::vec3(1.2f, 0.8f, 1.2f),
//         colorArticulacion, modelLoc);

//         glm::mat4 renderPalma = glm::translate(modMuneca,
//         glm::vec3(0.0f, 1.0f, 0.0f)); DibujarGeometria(renderPalma,
//         glm::vec3(2.5f, 2.0f, 0.6f), glm::vec3(0.7f), modelLoc);

//         glm::mat4 soportePulgar = glm::translate(modMuneca, glm::vec3(-1.25f,
//         -0.3f, 0.0f)); DibujarGeometria(soportePulgar, glm::vec3(0.6f, 1.0f,
//         0.6f), glm::vec3(0.7f), modelLoc);

//         glm::mat4 baseMano = pila.top();

//         DibujarDedoDetallado(baseMano, flexIndice,  -0.9f, 2.0f, 0.35f, 1.0f,
//         modelLoc); DibujarDedoDetallado(baseMano, flexMedio,   -0.3f, 2.0f,
//         0.38f, 1.1f, modelLoc); DibujarDedoDetallado(baseMano, flexAnular,
//         0.3f, 2.0f, 0.35f, 1.0f, modelLoc); DibujarDedoDetallado(baseMano,
//         flexMenique,  0.9f, 2.0f, 0.30f, 0.8f, modelLoc);

//         glm::mat4 mPulgar = baseMano;
//         mPulgar = glm::translate(mPulgar, glm::vec3(-1.3f, 0.2f, 0.0f));
//         mPulgar = glm::rotate(mPulgar, glm::radians(45.0f), glm::vec3(0.0f,
//         0.0f, 1.0f)); mPulgar = glm::rotate(mPulgar, glm::radians(-30.0f),
//         glm::vec3(0.0f, 1.0f, 0.0f)); DibujarDedoDetallado(mPulgar,
//         flexPulgar, 0.0f, 0.0f, 0.40f, 0.9f, modelLoc);

//         pila.pop(); pila.pop(); pila.pop(); pila.pop();

//         //
//         =====================================================================
//         // LAMPARAS (.obj)
//         //
//         =====================================================================
//         glUniform1i(glGetUniformLocation(lightingShader.Program,
//         "useTexture"), 1);

//         lampShader.Use();
//         GLint lampModelLoc = glGetUniformLocation(lampShader.Program,
//         "model"); glUniformMatrix4fv(glGetUniformLocation(lampShader.Program,
//         "view"), 1, GL_FALSE, glm::value_ptr(view));
//         glUniformMatrix4fv(glGetUniformLocation(lampShader.Program,
//         "projection"), 1, GL_FALSE, glm::value_ptr(projection));

//         for (int i = 0; i < 6; i++) {
//             glm::mat4 modelLamp = glm::translate(glm::mat4(1.0f),
//             posLamp1[i]); glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE,
//             glm::value_ptr(modelLamp)); Lampara1.Draw(lampShader);
//         }

//         glm::vec3 scL2Small(0.3135f, 0.1175f, 0.111f);
//         for (int i = 0; i < 2; i++) {
//             glm::mat4 modelLamp = glm::translate(glm::mat4(1.0f),
//             posLamp2[i]); modelLamp = glm::scale(modelLamp, scL2Small);
//             glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE,
//             glm::value_ptr(modelLamp)); Lampara2.Draw(lampShader);
//         }
//         for (int i = 2; i < 5; i++) {
//             glm::mat4 modelLamp = glm::translate(glm::mat4(1.0f),
//             posLamp2[i]); modelLamp = glm::rotate(modelLamp,
//             glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); modelLamp =
//             glm::scale(modelLamp, scL2Small);
//             glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE,
//             glm::value_ptr(modelLamp)); Lampara2.Draw(lampShader);
//         }

//         glBindVertexArray(0);

//         //
//         =====================================================================
//         // SKYBOX
//         //
//         =====================================================================
//         glDepthFunc(GL_LEQUAL);
//         skyboxShader.Use();
//         glm::mat4 skyView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
//         glUniformMatrix4fv(glGetUniformLocation(skyboxShader.Program,
//         "view"), 1, GL_FALSE, glm::value_ptr(skyView));
//         glUniformMatrix4fv(glGetUniformLocation(skyboxShader.Program,
//         "projection"), 1, GL_FALSE, glm::value_ptr(projection));
//         glBindVertexArray(skyboxVAO);
//         glActiveTexture(GL_TEXTURE1);
//         glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
//         glDrawArrays(GL_TRIANGLES, 0, 36);
//         glBindVertexArray(0);
//         glDepthFunc(GL_LESS);

//         glfwSwapBuffers(window);
//     }

//     glDeleteVertexArrays(1, &VAO);
//     glDeleteBuffers(1, &VBO);
//     glDeleteBuffers(1, &EBO);
//     glDeleteVertexArrays(1, &skyboxVAO);
//     glDeleteBuffers(1, &skyboxVBO);
//     glfwTerminate();

//     return 0;
// }

// //
// =============================================================================
// // CONTROLES Y CALLBACKS
// //
// =============================================================================
// void DoMovement() {
//     if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
//     camera.ProcessKeyboard(FORWARD, deltaTime); if (keys[GLFW_KEY_S] ||
//     keys[GLFW_KEY_DOWN])  camera.ProcessKeyboard(BACKWARD, deltaTime); if
//     (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])  camera.ProcessKeyboard(LEFT,
//     deltaTime); if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
//     camera.ProcessKeyboard(RIGHT, deltaTime);
// }

// void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int
// mode) {
//     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
//         glfwSetWindowShouldClose(window, GL_TRUE);

//     // B: encender/apagar lamparas
//     if (key == GLFW_KEY_B && action == GLFW_PRESS) {
//         lampsOn = !lampsOn;
//         std::cout << "Lamparas: " << (lampsOn ? "ON" : "OFF") << std::endl;
//     }

//     // G: encender/apagar animacion de GRUA + BRAZO
//     if (key == GLFW_KEY_G && action == GLFW_PRESS) {
//         gruaBrazoAnimOn = !gruaBrazoAnimOn;
//         std::cout << "Animacion Grua+Brazo: " << (gruaBrazoAnimOn ? "ON" :
//         "OFF") << std::endl;
//     }

//     // N: encender/apagar animacion de PERSONAJE
//     if (key == GLFW_KEY_N && action == GLFW_PRESS) {
//         personajeAnimOn = !personajeAnimOn;
//         std::cout << "Animacion Personaje: " << (personajeAnimOn ? "ON" :
//         "OFF") << std::endl;
//     }

//     // H: encender/apagar animacion del HELICOPTERO
//     if (key == GLFW_KEY_H && action == GLFW_PRESS) {
//         helicopteroAnimOn = !helicopteroAnimOn;
//         std::cout << "Animacion Helicoptero: " << (helicopteroAnimOn ? "ON" :
//         "OFF") << std::endl;
//     }

//     if (key >= 0 && key < 1024) {
//         if (action == GLFW_PRESS)   keys[key] = true;
//         if (action == GLFW_RELEASE) keys[key] = false;
//     }
// }

// void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
//     if (firstMouse) { lastX = (float)xPos; lastY = (float)yPos; firstMouse =
//     false; } GLfloat xOffset = (float)(xPos - lastX); GLfloat yOffset =
//     (float)(lastY - yPos); lastX = (float)xPos; lastY = (float)yPos;
//     camera.ProcessMouseMovement(xOffset, yOffset);
// }

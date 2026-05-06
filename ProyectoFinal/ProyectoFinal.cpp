/* * Proyecto Final: Lobby del Auditorio de la Facultad de Ingeniería
 * Equipo 07
 * Fecha
 */

#include <iostream>
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

 // --- PROTOTIPOS ---
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void Animation();
void SaveAnimationToFile();
bool LoadAnimationFromFile(const std::string& filename);
void EnsureAnimationsFolder();

// --- CONFIGURACIÓN DE VENTANA Y CÁMARA ---
const GLuint WIDTH = 1280, HEIGHT = 720;
Camera camera(glm::vec3(0.0f, 2.0f, 10.0f));
GLfloat lastX = WIDTH / 2.0, lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;
GLfloat deltaTime = 0.0f, lastFrame = 0.0f;

// --- VARIABLES DE ANIMACIÓN (SISTEMA DE KEYFRAMES) ---
#define MAX_FRAMES 200
int i_max_steps = 150;
int i_curr_steps = 0;

typedef struct _frame {
    // Transformaciones Globales del Objeto Animado (ej. Personaje o Stand)
    float posX, posY, posZ;
    float rotX, rotY, rotZ;
    float scale;

    // Componentes Articulados (ej. Puertas del auditorio, brazos de un avatar)
    float articulacion1; // Ejemplo: Puerta Izquierda
    float articulacion2; // Ejemplo: Puerta Derecha
    float articulacion3; // Ejemplo: Elemento extra

    // Incrementos (Interpolación)
    float incX, incY, incZ;
    float incRotX, incRotY, incRotZ;
    float incArt1, incArt2, incArt3;

} FRAME;

FRAME KeyFrame[MAX_FRAMES];
int FrameIndex = 0;
bool play = false;
int playIndex = 0;
bool pendingSave = false, pendingLoad = false;

// Variables de control en tiempo real (las que modificas con el teclado)
float g_posX = 0, g_posY = 0, g_posZ = 0;
float g_rotY = 0;
float g_art1 = 0, g_art2 = 0;

// --- SECCIÓN DE MODELOS (TODO: Cargar aquí los elementos del Lobby) ---
Model* LobbyGeneral;
Model* StandModulo;
Model* PuertaL;
Model* PuertaR;

// --- LÓGICA DE KEYFRAMES (ADAPTADA) ---

void saveFrame(void) {
    printf("Guardando Frame: %d\n", FrameIndex);
    KeyFrame[FrameIndex].posX = g_posX;
    KeyFrame[FrameIndex].posY = g_posY;
    KeyFrame[FrameIndex].posZ = g_posZ;
    KeyFrame[FrameIndex].rotY = g_rotY;
    KeyFrame[FrameIndex].articulacion1 = g_art1;
    KeyFrame[FrameIndex].articulacion2 = g_art2;
    FrameIndex++;
}

void interpolation(void) {
    KeyFrame[playIndex].incX = (KeyFrame[playIndex + 1].posX - KeyFrame[playIndex].posX) / i_max_steps;
    KeyFrame[playIndex].incY = (KeyFrame[playIndex + 1].posY - KeyFrame[playIndex].posY) / i_max_steps;
    KeyFrame[playIndex].incZ = (KeyFrame[playIndex + 1].posZ - KeyFrame[playIndex].posZ) / i_max_steps;
    KeyFrame[playIndex].incRotY = (KeyFrame[playIndex + 1].rotY - KeyFrame[playIndex].rotY) / i_max_steps;
    KeyFrame[playIndex].incArt1 = (KeyFrame[playIndex + 1].articulacion1 - KeyFrame[playIndex].articulacion1) / i_max_steps;
    KeyFrame[playIndex].incArt2 = (KeyFrame[playIndex + 1].articulacion2 - KeyFrame[playIndex].articulacion2) / i_max_steps;
}

// --- MAIN LOOP ---
int main() {
    EnsureAnimationsFolder();
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "CGIH 2026: Proyecto Lobby Auditorio", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glewExperimental = GL_TRUE;
    glewInit();
    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    Shader mainShader("Shader/lighting.vs", "Shader/lighting.frag");

    // TODO: Inicializar modelos del proyecto final
    // LobbyGeneral = new Model((char*)"Models/Lobby/lobby.obj");

    while (!glfwWindowShouldClose(window)) {
        GLfloat currentFrame = (GLfloat)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        DoMovement();
        Animation();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mainShader.Use();

        // --- MATRICES DE CÁMARA ---
        glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)WIDTH / HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(mainShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(mainShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // --- RENDER DEL ESCENARIO (LOBBY FI) ---
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(mainShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
        // LobbyGeneral->Draw(mainShader);

        // --- RENDER DE ELEMENTOS ANIMADOS (Ejemplo: Puertas o Stands) ---
        model = glm::translate(glm::mat4(1.0f), glm::vec3(g_posX, g_posY, g_posZ));
        model = glm::rotate(model, glm::radians(g_rotY), glm::vec3(0, 1, 0));
        // TODO: Aquí aplicar g_art1 o g_art2 a las piezas móviles del modelo
        glUniformMatrix4fv(glGetUniformLocation(mainShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
        // PuertaL->Draw(mainShader);

        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}

// --- SISTEMA DE MOVIMIENTO Y CONTROL ---
void DoMovement() {
    // Navegación de cámara estándar
    if (keys[GLFW_KEY_W]) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S]) camera.ProcessKeyboard(BACKWARD, deltaTime);

    // Controles para posicionar stands o animar objetos del Lobby
    if (keys[GLFW_KEY_LEFT])  g_posX -= 0.01f;
    if (keys[GLFW_KEY_RIGHT]) g_posX += 0.01f;
    if (keys[GLFW_KEY_UP])    g_posZ -= 0.01f;
    if (keys[GLFW_KEY_DOWN])  g_posZ += 0.01f;

    // Control de articulación (Ej. Abrir puertas)
    if (keys[GLFW_KEY_O]) g_art1 += 0.5f; // Open
    if (keys[GLFW_KEY_C]) g_art1 -= 0.5f; // Close
}

void Animation() {
    if (play) {
        if (i_curr_steps >= i_max_steps) {
            playIndex++;
            if (playIndex > FrameIndex - 2) {
                play = false;
                playIndex = 0;
            }
            else {
                i_curr_steps = 0;
                interpolation();
            }
        }
        else {
            g_posX += KeyFrame[playIndex].incX;
            g_posY += KeyFrame[playIndex].incY;
            g_posZ += KeyFrame[playIndex].incZ;
            g_rotY += KeyFrame[playIndex].incRotY;
            g_art1 += KeyFrame[playIndex].incArt1;
            g_art2 += KeyFrame[playIndex].incArt2;
            i_curr_steps++;
        }
    }
}

void DoMovement()
{

	// Camera controls
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])    camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])   camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])   camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])  camera.ProcessKeyboard(RIGHT, deltaTime);
}

// Is called whenever a key is pressed/released via GLFW
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{


	// Guardar keyframe manualmente (K)
	if (key == GLFW_KEY_K && action == GLFW_PRESS)
	{
		if (FrameIndex < MAX_FRAMES)
		{
			saveFrame();
		}
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		pendingSave = true;
	}


	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		pendingLoad = true;
	}


	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			keys[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			keys[key] = false;
		}
	}

}




void MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}

	GLfloat xOffset = xPos - lastX;
	GLfloat yOffset = lastY - yPos;

	lastX = xPos;
	lastY = yPos;

	camera.ProcessMouseMovement(xOffset, yOffset);
}
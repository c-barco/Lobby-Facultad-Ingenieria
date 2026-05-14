// Modelo Jerárquico: Brazo Completo y Mano Robótica con Cámara Libre y Animación
// Nivel de detalle: Geometría Compuesta, Animación KeyFrames (Suavizada) y Ciclo Pick-and-Place
// VERSIÓN EXHIBICIÓN: Auto-Play y Bucle Infinito

#include <iostream>
#include <fstream>
#include <stack>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Dependencias
#include "Shader.h"
#include "Camera.h"

// Prototipos de funciones
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void Animation();
void resetElements();
void loadFromFile();

const GLint WIDTH = 1200, HEIGHT = 800;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Configuración de la Cámara Libre
Camera camera(glm::vec3(0.0f, 5.0f, 20.0f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Variables de articulaciones del modelo
float baseY = 0.0f;
float hombroX = 0.0f, hombroZ = 0.0f;
float codo = 0.0f;
float munecaX = 0.0f, munecaZ = 0.0f;

// Variables para la flexión de los dedos
float flexPulgar = 0.0f;
float flexIndice = 0.0f;
float flexMedio = 0.0f;
float flexAnular = 0.0f;
float flexMenique = 0.0f;

// ================= SISTEMA DE ANIMACIÓN (KEYFRAMES) =================
#define MAX_FRAMES 20
int i_max_steps = 120; // Velocidad de la animación
int i_curr_steps = 0;

typedef struct _frame {
    float baseY;
    float hombroX, hombroZ;
    float codo;
    float munecaX, munecaZ;
    float flexPulgar, flexIndice, flexMedio, flexAnular, flexMenique;
} FRAME;

FRAME KeyFrame[MAX_FRAMES];
int FrameIndex = 0;
bool play = false;
int playIndex = 0;

void resetElements(void) {
    if (FrameIndex > 0) {
        baseY = KeyFrame[0].baseY;
        hombroX = KeyFrame[0].hombroX;
        hombroZ = KeyFrame[0].hombroZ;
        codo = KeyFrame[0].codo;
        munecaX = KeyFrame[0].munecaX;
        munecaZ = KeyFrame[0].munecaZ;
        flexPulgar = KeyFrame[0].flexPulgar;
        flexIndice = KeyFrame[0].flexIndice;
        flexMedio = KeyFrame[0].flexMedio;
        flexAnular = KeyFrame[0].flexAnular;
        flexMenique = KeyFrame[0].flexMenique;
    }
}

void loadFromFile() {
    std::ifstream file("animacion_pick_place.txt");
    if (file.is_open()) {
        file >> FrameIndex;
        for (int i = 0; i < FrameIndex; i++) {
            file >> KeyFrame[i].baseY >> KeyFrame[i].hombroX >> KeyFrame[i].hombroZ
                >> KeyFrame[i].codo >> KeyFrame[i].munecaX >> KeyFrame[i].munecaZ
                >> KeyFrame[i].flexPulgar >> KeyFrame[i].flexIndice
                >> KeyFrame[i].flexMedio >> KeyFrame[i].flexAnular >> KeyFrame[i].flexMenique;
        }
        file.close();
        std::cout << "¡Animación cargada! Frames totales: " << FrameIndex << std::endl;
        resetElements();
    }
    else {
        std::cout << "Error: No se encontró el archivo 'animacion_pick_place.txt'" << std::endl;
    }
}

// FUNCIÓN DE ANIMACIÓN CON BUCLE INFINITO
void Animation() {
    if (play && FrameIndex > 1) {
        if (i_curr_steps >= i_max_steps) {
            playIndex++;
            if (playIndex > FrameIndex - 2) {
                // REINICIO AUTOMÁTICO (LOOP)
                playIndex = 0;
                i_curr_steps = 0;
                resetElements();
            }
            else {
                i_curr_steps = 0;
            }
        }
        else {
            float t = (float)i_curr_steps / (float)i_max_steps;
            float smooth_t = t * t * (3.0f - 2.0f * t);

            baseY = KeyFrame[playIndex].baseY + (KeyFrame[playIndex + 1].baseY - KeyFrame[playIndex].baseY) * smooth_t;
            hombroX = KeyFrame[playIndex].hombroX + (KeyFrame[playIndex + 1].hombroX - KeyFrame[playIndex].hombroX) * smooth_t;
            hombroZ = KeyFrame[playIndex].hombroZ + (KeyFrame[playIndex + 1].hombroZ - KeyFrame[playIndex].hombroZ) * smooth_t;
            codo = KeyFrame[playIndex].codo + (KeyFrame[playIndex + 1].codo - KeyFrame[playIndex].codo) * smooth_t;
            munecaX = KeyFrame[playIndex].munecaX + (KeyFrame[playIndex + 1].munecaX - KeyFrame[playIndex].munecaX) * smooth_t;
            munecaZ = KeyFrame[playIndex].munecaZ + (KeyFrame[playIndex + 1].munecaZ - KeyFrame[playIndex].munecaZ) * smooth_t;

            flexPulgar = KeyFrame[playIndex].flexPulgar + (KeyFrame[playIndex + 1].flexPulgar - KeyFrame[playIndex].flexPulgar) * smooth_t;
            flexIndice = KeyFrame[playIndex].flexIndice + (KeyFrame[playIndex + 1].flexIndice - KeyFrame[playIndex].flexIndice) * smooth_t;
            flexMedio = KeyFrame[playIndex].flexMedio + (KeyFrame[playIndex + 1].flexMedio - KeyFrame[playIndex].flexMedio) * smooth_t;
            flexAnular = KeyFrame[playIndex].flexAnular + (KeyFrame[playIndex + 1].flexAnular - KeyFrame[playIndex].flexAnular) * smooth_t;
            flexMenique = KeyFrame[playIndex].flexMenique + (KeyFrame[playIndex + 1].flexMenique - KeyFrame[playIndex].flexMenique) * smooth_t;

            i_curr_steps++;
        }
    }
}

// ================= FUNCIÓN PRINCIPAL =================
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Brazo Robotico - Animacion Pick and Place", nullptr, nullptr);
    glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

    if (nullptr == window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    glewInit();

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader ourShader("Shader/core.vs", "Shader/core.frag");

    float vertices[] = {
        -0.5f, -0.5f, 0.5f,  0.5f, -0.5f, 0.5f,  0.5f,  0.5f, 0.5f,
        0.5f,  0.5f, 0.5f, -0.5f,  0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f,-0.5f,  0.5f, -0.5f,-0.5f,  0.5f,  0.5f,-0.5f,
        0.5f,  0.5f,-0.5f, -0.5f,  0.5f,-0.5f, -0.5f, -0.5f,-0.5f,
         0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

    GLint uniformColorLoc = ourShader.uniformColor;
    GLint modelLocTemp;

    auto DibujarGeometria = [&](glm::mat4 mBase, glm::vec3 escala, glm::vec3 color) {
        glm::mat4 mForma = glm::scale(mBase, escala);
        glUniform3fv(uniformColorLoc, 1, glm::value_ptr(color));
        glUniformMatrix4fv(modelLocTemp, 1, GL_FALSE, glm::value_ptr(mForma));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        };

    auto DibujarDedoDetallado = [&](glm::mat4 baseDedo, float flexion, float offsetX, float offsetY, float scaleGrosor, float scaleLargo) {
        glm::mat4 mDedo = baseDedo;
        glm::vec3 colorNudillo(0.15f, 0.15f, 0.15f);
        glm::vec3 colorFalange(0.8f, 0.8f, 0.8f);

        mDedo = glm::translate(mDedo, glm::vec3(offsetX, offsetY, 0.0f));
        mDedo = glm::rotate(mDedo, glm::radians(flexion), glm::vec3(1.0f, 0.0f, 0.0f));
        DibujarGeometria(mDedo, glm::vec3(scaleGrosor * 1.2f), colorNudillo);

        glm::mat4 mRender1 = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo / 2, 0.0f));
        DibujarGeometria(mRender1, glm::vec3(scaleGrosor, scaleLargo, scaleGrosor), colorFalange);

        mDedo = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo, 0.0f));
        mDedo = glm::rotate(mDedo, glm::radians(flexion * 0.8f), glm::vec3(1.0f, 0.0f, 0.0f));
        DibujarGeometria(mDedo, glm::vec3(scaleGrosor * 1.1f), colorNudillo);

        glm::mat4 mRender2 = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo * 0.45f, 0.0f));
        DibujarGeometria(mRender2, glm::vec3(scaleGrosor * 0.9f, scaleLargo * 0.9f, scaleGrosor * 0.9f), colorFalange);

        mDedo = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo * 0.9f, 0.0f));
        mDedo = glm::rotate(mDedo, glm::radians(flexion * 0.6f), glm::vec3(1.0f, 0.0f, 0.0f));
        DibujarGeometria(mDedo, glm::vec3(scaleGrosor * 0.9f), colorNudillo);

        glm::mat4 mRender3 = glm::translate(mDedo, glm::vec3(0.0f, scaleLargo * 0.35f, 0.0f));
        DibujarGeometria(mRender3, glm::vec3(scaleGrosor * 0.8f, scaleLargo * 0.7f, scaleGrosor * 0.8f), colorFalange);
        };

    // --- ARRANQUE AUTOMÁTICO ---
    loadFromFile();
    if (FrameIndex > 1) {
        play = true; // Empieza a reproducir de inmediato
    }

    while (!glfwWindowShouldClose(window)) {
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        DoMovement();
        Animation();

        glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.Use();

        glm::mat4 view = camera.GetViewMatrix();
        GLint modelLoc = glGetUniformLocation(ourShader.Program, "model");
        modelLocTemp = modelLoc;
        GLint viewLoc = glGetUniformLocation(ourShader.Program, "view");
        GLint projecLoc = glGetUniformLocation(ourShader.Program, "projection");
        uniformColorLoc = ourShader.uniformColor;

        glUniformMatrix4fv(projecLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(VAO);

        // --- DIBUJAR OBJETO OBJETIVO ---
        glm::mat4 mPieza = glm::mat4(1.0f);
        mPieza = glm::translate(mPieza, glm::vec3(3.5f, -4.5f, 2.0f));
        DibujarGeometria(mPieza, glm::vec3(1.2f, 1.2f, 1.2f), glm::vec3(0.9f, 0.2f, 0.2f));

        // --- JERARQUÍA DEL BRAZO ROBÓTICO ---
        std::stack<glm::mat4> pila;
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 colorBrazo(0.6f, 0.6f, 0.65f);
        glm::vec3 colorArticulacion(0.15f, 0.15f, 0.15f);

        model = glm::translate(model, glm::vec3(0.0f, -5.0f, 0.0f));
        model = glm::rotate(model, glm::radians(baseY), glm::vec3(0.0f, 1.0f, 0.0f));
        pila.push(model);
        DibujarGeometria(model, glm::vec3(3.0f, 0.5f, 3.0f), colorArticulacion);

        model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(hombroX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(hombroZ), glm::vec3(0.0f, 0.0f, 1.0f));
        pila.push(model);

        DibujarGeometria(model, glm::vec3(1.5f, 1.5f, 1.5f), colorArticulacion);

        glm::mat4 renderBrazo = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
        DibujarGeometria(renderBrazo, glm::vec3(1.2f, 4.0f, 1.2f), colorBrazo);

        model = pila.top();
        model = glm::translate(model, glm::vec3(0.0f, 4.0f, 0.0f));
        model = glm::rotate(model, glm::radians(codo), glm::vec3(1.0f, 0.0f, 0.0f));
        pila.push(model);

        DibujarGeometria(model, glm::vec3(1.4f, 1.2f, 1.4f), colorArticulacion);

        glm::mat4 barraIzq = glm::translate(model, glm::vec3(-0.4f, 2.0f, 0.0f));
        DibujarGeometria(barraIzq, glm::vec3(0.3f, 4.0f, 0.8f), colorBrazo);

        glm::mat4 barraDer = glm::translate(model, glm::vec3(0.4f, 2.0f, 0.0f));
        DibujarGeometria(barraDer, glm::vec3(0.3f, 4.0f, 0.8f), colorBrazo);

        glm::mat4 piston = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
        DibujarGeometria(piston, glm::vec3(0.2f, 3.8f, 0.2f), glm::vec3(0.2f));

        model = pila.top();
        model = glm::translate(model, glm::vec3(0.0f, 4.0f, 0.0f));
        model = glm::rotate(model, glm::radians(munecaX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(munecaZ), glm::vec3(0.0f, 0.0f, 1.0f));
        pila.push(model);

        DibujarGeometria(model, glm::vec3(1.2f, 0.8f, 1.2f), colorArticulacion);

        glm::mat4 renderPalma = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f));
        DibujarGeometria(renderPalma, glm::vec3(2.5f, 2.0f, 0.6f), glm::vec3(0.7f));

        glm::mat4 soportePulgar = glm::translate(model, glm::vec3(-1.25f, -0.3f, 0.0f));
        DibujarGeometria(soportePulgar, glm::vec3(0.6f, 1.0f, 0.6f), glm::vec3(0.7f));

        glm::mat4 baseMano = pila.top();

        DibujarDedoDetallado(baseMano, flexIndice, -0.9f, 2.0f, 0.35f, 1.0f);
        DibujarDedoDetallado(baseMano, flexMedio, -0.3f, 2.0f, 0.38f, 1.1f);
        DibujarDedoDetallado(baseMano, flexAnular, 0.3f, 2.0f, 0.35f, 1.0f);
        DibujarDedoDetallado(baseMano, flexMenique, 0.9f, 2.0f, 0.30f, 0.8f);

        glm::mat4 mPulgar = baseMano;
        mPulgar = glm::translate(mPulgar, glm::vec3(-1.3f, 0.2f, 0.0f));
        mPulgar = glm::rotate(mPulgar, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mPulgar = glm::rotate(mPulgar, glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        DibujarDedoDetallado(mPulgar, flexPulgar, 0.0f, 0.0f, 0.40f, 0.9f);

        pila.pop();
        pila.pop();
        pila.pop();
        pila.pop();

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return EXIT_SUCCESS;
}

// ================= CONTROLES (Cámara solamente) =================
void DoMovement() {
    // WASD para mover la cámara libremente
    if (keys[GLFW_KEY_W]) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S]) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A]) camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D]) camera.ProcessKeyboard(RIGHT, deltaTime);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) glfwSetWindowShouldClose(window, GL_TRUE);

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
    if (firstMouse) { lastX = xPos; lastY = yPos; firstMouse = false; }
    camera.ProcessMouseMovement(xPos - lastX, lastY - yPos);
    lastX = xPos; lastY = yPos;
}
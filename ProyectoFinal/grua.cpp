// Proyecto Final: Exhibición Automática de Grúa Torre (Bucle Infinito)
// Hernández Juárez Fernando - 320115448

#include <iostream>
#include <stack>
#include <cmath>
#include <fstream> 
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"

// Prototipos
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement(); // Maneja la cámara
void Animation();
void resetElements();
void interpolation();
void loadFromFile();

const GLuint WIDTH = 1200, HEIGHT = 800;
int SCREEN_WIDTH, SCREEN_HEIGHT;

Camera camera(glm::vec3(0.0f, 15.0f, 40.0f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// ==========================================
// VARIABLES DE LA GRÚA
// ==========================================
float gruaPosX = 0.0f;
float gruaPosY = 0.0f;
float gruaPosZ = 0.0f;

float rotTorreta = 0.0f;
float elevacionPluma = 25.0f;
float extensionTelescopio = 0.0f;
float largoCableActual = 6.0f;

// Física (Péndulo) - Se mantienen para la lectura del archivo
float tiempoOscilacion = 0.0f;
float amplitudActualX = 0.0f;
float amplitudActualZ = 0.0f;
float anguloPenduloX = 0.0f;
float anguloPenduloZ = 0.0f;
bool enMovimiento = false;

// ==========================================
// SISTEMA DE ANIMACIÓN (AUTOMATIZADO)
// ==========================================
#define MAX_FRAMES 100 
int i_max_steps = 190;
int i_curr_steps = 0;

typedef struct _frame {
    float gruaPosX, gruaPosY, gruaPosZ;
    float rotTorreta, elevacionPluma, extensionTelescopio, largoCableActual;
    float anguloPenduloX, anguloPenduloZ;

    float incX, incY, incZ;
    float rotTorretaInc, elevacionPlumaInc, extensionTelescopioInc, largoCableActualInc;
    float anguloPenduloXInc, anguloPenduloZInc;
} FRAME;

FRAME KeyFrame[MAX_FRAMES];
int FrameIndex = 0;
bool play = false;
int playIndex = 0;


int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Feria Estudiantil - Modo Exhibicion Automatica", nullptr, nullptr);
    glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

    if (nullptr == window) {
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
         0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,
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

    for (int i = 0; i < MAX_FRAMES; i++) {
        KeyFrame[i] = { 0 };
    }

    // ==========================================
    // CARGA Y REPRODUCCIÓN AUTOMÁTICA AL INICIAR
    // ==========================================
    loadFromFile();
    if (FrameIndex > 1) {
        resetElements();
        interpolation();
        play = true;
        playIndex = 0;
        i_curr_steps = 0;
        printf("Reproduciendo animacion en Bucle Infinito...\n");
    }
    else {
        printf("ADVERTENCIA: No se encontro animacion suficiente en el archivo.\n");
    }

    while (!glfwWindowShouldClose(window)) {
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        DoMovement(); // Solo mueve la cámara

        // Ejecuta la animación si está activa
        if (play) {
            Animation();
        }

        glClearColor(0.2f, 0.3f, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.Use();
        glm::mat4 view = camera.GetViewMatrix();
        GLint modelLoc = glGetUniformLocation(ourShader.Program, "model");
        GLint viewLoc = glGetUniformLocation(ourShader.Program, "view");
        GLint projecLoc = glGetUniformLocation(ourShader.Program, "projection");
        GLint uniformColor = ourShader.uniformColor;

        glUniformMatrix4fv(projecLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(VAO);
        std::stack<glm::mat4> pila;
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 color;

        // 1. CIMIENTOS Y JERARQUÍA GLOBAL
        glm::mat4 modelBase = translate(model, glm::vec3(gruaPosX, gruaPosY, gruaPosZ));

        glm::mat4 renderCimiento = scale(modelBase, glm::vec3(3.0f, 0.5f, 3.0f));
        color = glm::vec3(0.3f, 0.3f, 0.3f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderCimiento));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        int numSegmentos = 6;
        float altoSeg = 1.5f;
        float anchoSeg = 1.2f;
        float grosor = 0.1f;

        color = glm::vec3(0.9f, 0.7f, 0.1f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));

        for (int i = 0; i < numSegmentos; i++) {
            glm::mat4 baseSeg = translate(modelBase, glm::vec3(0.0f, 0.25f + (i * altoSeg), 0.0f));
            for (int x : {-1, 1}) {
                for (int z : {-1, 1}) {
                    glm::mat4 poste = translate(baseSeg, glm::vec3(x * anchoSeg / 2, altoSeg / 2, z * anchoSeg / 2));
                    poste = scale(poste, glm::vec3(grosor, altoSeg, grosor));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(poste));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
            for (int z : {-1, 1}) {
                glm::mat4 horiz = translate(baseSeg, glm::vec3(0.0f, altoSeg, z * anchoSeg / 2));
                horiz = scale(horiz, glm::vec3(anchoSeg + grosor, grosor, grosor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(horiz));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            for (int x : {-1, 1}) {
                glm::mat4 horiz = translate(baseSeg, glm::vec3(x * anchoSeg / 2, altoSeg, 0.0f));
                horiz = scale(horiz, glm::vec3(grosor, grosor, anchoSeg + grosor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(horiz));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            float anguloDiag = atan2(altoSeg, anchoSeg);
            float largoDiag = sqrt(altoSeg * altoSeg + anchoSeg * anchoSeg);
            for (int z : {-1, 1}) {
                glm::mat4 diag = translate(baseSeg, glm::vec3(0.0f, altoSeg / 2, z * anchoSeg / 2));
                diag = rotate(diag, (z == 1) ? anguloDiag : -anguloDiag, glm::vec3(0.0f, 0.0f, 1.0f));
                diag = scale(diag, glm::vec3(largoDiag, grosor / 2, grosor / 2));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(diag));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // 2. TORRETA GIRATORIA Y CABINA
        float alturaTorre = 0.25f + (numSegmentos * altoSeg);
        glm::mat4 modelTorreta = translate(modelBase, glm::vec3(0.0f, alturaTorre + 0.2f, 0.0f));
        modelTorreta = rotate(modelTorreta, glm::radians(rotTorreta), glm::vec3(0.0f, 1.0f, 0.0f));
        pila.push(modelTorreta);

        glm::mat4 renderTorreta = scale(modelTorreta, glm::vec3(2.0f, 0.4f, 2.0f));
        color = glm::vec3(0.2f, 0.2f, 0.2f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderTorreta));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 renderCabina = translate(modelTorreta, glm::vec3(-0.9f, -0.6f, 0.9f));
        renderCabina = scale(renderCabina, glm::vec3(0.8f, 1.5f, 1.0f));
        color = glm::vec3(0.1f, 0.4f, 0.7f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderCabina));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 renderTrasero = translate(modelTorreta, glm::vec3(0.0f, 0.3f, -1.2f));
        renderTrasero = scale(renderTrasero, glm::vec3(1.5f, 0.8f, 1.5f));
        color = glm::vec3(0.15f, 0.15f, 0.15f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderTrasero));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 3. MÁSTIL SUPERIOR Y PLUMAS
        glm::mat4 modelMastil = translate(modelTorreta, glm::vec3(0.0f, 1.5f, 0.0f));
        pila.push(modelMastil);

        glm::mat4 renderMastil = scale(modelMastil, glm::vec3(0.8f, 3.0f, 0.8f));
        color = glm::vec3(0.9f, 0.7f, 0.1f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderMastil));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 modelPlumaBase = pila.top();
        modelPlumaBase = translate(modelPlumaBase, glm::vec3(0.0f, 0.0f, 0.5f));
        modelPlumaBase = rotate(modelPlumaBase, glm::radians(elevacionPluma), glm::vec3(1.0f, 0.0f, 0.0f));
        pila.push(modelPlumaBase);

        glm::mat4 renderPluma = translate(modelPlumaBase, glm::vec3(0.0f, 0.0f, 4.0f));
        renderPluma = scale(renderPluma, glm::vec3(0.8f, 0.8f, 8.0f));
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderPluma));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 4. SEGMENTO TELESCÓPICO 
        glm::mat4 modelTeleNode = pila.top();

        modelTeleNode = translate(modelTeleNode, glm::vec3(0.0f, 0.0f, 1.0f + extensionTelescopio));
        pila.push(modelTeleNode);

        glm::mat4 renderTele = translate(modelTeleNode, glm::vec3(0.0f, 0.0f, 4.0f));
        renderTele = scale(renderTele, glm::vec3(0.6f, 0.6f, 8.0f));
        color = glm::vec3(0.7f, 0.7f, 0.7f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderTele));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 5. CARGA OSCILANTE Y CABLES 
        glm::mat4 modelAnclajeGiro = translate(modelTeleNode, glm::vec3(0.0f, -0.3f, 8.0f));
        pila.pop();

        modelAnclajeGiro = rotate(modelAnclajeGiro, glm::radians(-elevacionPluma), glm::vec3(1.0f, 0.0f, 0.0f));
        modelAnclajeGiro = rotate(modelAnclajeGiro, glm::radians(anguloPenduloX), glm::vec3(1.0f, 0.0f, 0.0f));
        modelAnclajeGiro = rotate(modelAnclajeGiro, glm::radians(anguloPenduloZ), glm::vec3(0.0f, 0.0f, 1.0f));

        float conWidth = 3.6f;
        float conHeight = 1.2f;
        float conDepth = 1.2f;
        float spreadX = conWidth / 2.0f;

        glm::mat4 renderSpreaderTop = scale(modelAnclajeGiro, glm::vec3(0.4f, 0.4f, 0.4f));
        color = glm::vec3(0.1f, 0.1f, 0.1f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderSpreaderTop));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        float diagLength = sqrt((spreadX * spreadX) + (largoCableActual * largoCableActual));
        float angleCable = atan2(spreadX, largoCableActual);

        color = glm::vec3(0.0f, 0.0f, 0.0f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));

        for (int i = -1; i <= 1; i += 2) {
            float xDir = (float)i;
            glm::mat4 renderCable = translate(modelAnclajeGiro, glm::vec3(xDir * spreadX / 2.0f, -largoCableActual / 2.0f, 0.0f));
            renderCable = rotate(renderCable, xDir * angleCable, glm::vec3(0.0f, 0.0f, 1.0f));
            renderCable = scale(renderCable, glm::vec3(0.04f, diagLength, 0.04f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderCable));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glm::mat4 modelContenedorBase = translate(modelAnclajeGiro, glm::vec3(0.0f, -largoCableActual - (conHeight / 2.0f), 0.0f));

        glm::mat4 renderCuerpoCon = scale(modelContenedorBase, glm::vec3(conWidth, conHeight, conDepth));
        color = glm::vec3(0.5f, 0.15f, 0.1f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderCuerpoCon));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        int numRibs = 14;
        float ribGrosor = 0.06f;
        float ribSaliente = 0.05f;
        color = glm::vec3(0.4f, 0.1f, 0.05f);
        glUniform3fv(uniformColor, 1, glm::value_ptr(color));

        for (int i = 0; i < numRibs; i++) {
            float xPosRib = -conWidth / 2.0f + (i * conWidth / (numRibs - 1));

            glm::mat4 renderRibFrente = translate(modelContenedorBase, glm::vec3(xPosRib, 0.0f, conDepth / 2.0f + ribSaliente / 2.0f));
            renderRibFrente = scale(renderRibFrente, glm::vec3(ribGrosor, conHeight + 0.01f, ribSaliente));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderRibFrente));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glm::mat4 renderRibAtras = translate(modelContenedorBase, glm::vec3(xPosRib, 0.0f, -conDepth / 2.0f - ribSaliente / 2.0f));
            renderRibAtras = scale(renderRibAtras, glm::vec3(ribGrosor, conHeight + 0.01f, ribSaliente));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(renderRibAtras));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

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

// =========================================================
// MOVIMIENTO DE CÁMARA (ÚNICO CONTROL MANUAL ACTIVO)
// =========================================================

void DoMovement() {
    if (keys[GLFW_KEY_W]) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S]) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A]) camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D]) camera.ProcessKeyboard(RIGHT, deltaTime);
}

// =========================================================
// LÓGICA DE REPRODUCCIÓN (BUCLE INFINITO)
// =========================================================

void resetElements(void) {
    gruaPosX = KeyFrame[0].gruaPosX;
    gruaPosY = KeyFrame[0].gruaPosY;
    gruaPosZ = KeyFrame[0].gruaPosZ;
    rotTorreta = KeyFrame[0].rotTorreta;
    elevacionPluma = KeyFrame[0].elevacionPluma;
    extensionTelescopio = KeyFrame[0].extensionTelescopio;
    largoCableActual = KeyFrame[0].largoCableActual;
    anguloPenduloX = KeyFrame[0].anguloPenduloX;
    anguloPenduloZ = KeyFrame[0].anguloPenduloZ;
}

void interpolation(void) {
    KeyFrame[playIndex].incX = (KeyFrame[playIndex + 1].gruaPosX - KeyFrame[playIndex].gruaPosX) / i_max_steps;
    KeyFrame[playIndex].incY = (KeyFrame[playIndex + 1].gruaPosY - KeyFrame[playIndex].gruaPosY) / i_max_steps;
    KeyFrame[playIndex].incZ = (KeyFrame[playIndex + 1].gruaPosZ - KeyFrame[playIndex].gruaPosZ) / i_max_steps;
    KeyFrame[playIndex].rotTorretaInc = (KeyFrame[playIndex + 1].rotTorreta - KeyFrame[playIndex].rotTorreta) / i_max_steps;
    KeyFrame[playIndex].elevacionPlumaInc = (KeyFrame[playIndex + 1].elevacionPluma - KeyFrame[playIndex].elevacionPluma) / i_max_steps;
    KeyFrame[playIndex].extensionTelescopioInc = (KeyFrame[playIndex + 1].extensionTelescopio - KeyFrame[playIndex].extensionTelescopio) / i_max_steps;
    KeyFrame[playIndex].largoCableActualInc = (KeyFrame[playIndex + 1].largoCableActual - KeyFrame[playIndex].largoCableActual) / i_max_steps;
    KeyFrame[playIndex].anguloPenduloXInc = (KeyFrame[playIndex + 1].anguloPenduloX - KeyFrame[playIndex].anguloPenduloX) / i_max_steps;
    KeyFrame[playIndex].anguloPenduloZInc = (KeyFrame[playIndex + 1].anguloPenduloZ - KeyFrame[playIndex].anguloPenduloZ) / i_max_steps;
}

void loadFromFile() {
    std::ifstream file("animacion_grua.txt");
    if (file.is_open()) {
        file >> FrameIndex;
        for (int i = 0; i < FrameIndex; i++) {
            file >> KeyFrame[i].gruaPosX >> KeyFrame[i].gruaPosY >> KeyFrame[i].gruaPosZ
                >> KeyFrame[i].rotTorreta >> KeyFrame[i].elevacionPluma
                >> KeyFrame[i].extensionTelescopio >> KeyFrame[i].largoCableActual
                >> KeyFrame[i].anguloPenduloX >> KeyFrame[i].anguloPenduloZ;
        }
        file.close();
        printf("¡Animacion cargada correctamente!\n");
    }
    else {
        printf("ADVERTENCIA: No se encontro el archivo 'animacion_grua.txt'.\n");
    }
}

void Animation() {
    if (i_curr_steps >= i_max_steps) {
        playIndex++;
        if (playIndex > FrameIndex - 2) {
            // ¡EL BUCLE INFINITO SUCEDE AQUÍ!
            // En lugar de apagar (play = false), reinicia el índice y llama a resetElements()
            playIndex = 0;
            i_curr_steps = 0;
            resetElements();
            interpolation();
        }
        else {
            i_curr_steps = 0;
            interpolation();
        }
    }
    else {
        gruaPosX += KeyFrame[playIndex].incX;
        gruaPosY += KeyFrame[playIndex].incY;
        gruaPosZ += KeyFrame[playIndex].incZ;
        rotTorreta += KeyFrame[playIndex].rotTorretaInc;
        elevacionPluma += KeyFrame[playIndex].elevacionPlumaInc;
        extensionTelescopio += KeyFrame[playIndex].extensionTelescopioInc;
        largoCableActual += KeyFrame[playIndex].largoCableActualInc;
        anguloPenduloX += KeyFrame[playIndex].anguloPenduloXInc;
        anguloPenduloZ += KeyFrame[playIndex].anguloPenduloZInc;
        i_curr_steps++;
    }
}

// =========================================================
// CALLBACKS
// =========================================================

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
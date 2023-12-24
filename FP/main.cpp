#include <iostream>
#include <filesystem>
#include <cmath>
#include <iomanip>
#include <ctime>
#include <random>

// #define VSYNC_DISABLED
#define DEFERRED_SHADING
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#pragma comment(lib, "glfw3dll.lib")
#pragma comment(lib, "assimp-vc143-mt.lib")

#include "model.h"
#include "mouse.h"
#include "camera.h"
#include "fbo.h"
#include "gbo.h"
#include "shader/shader.h"
#include "texture/texture.h"

using namespace std;
using namespace glm;
namespace fs = std::filesystem;



// Global variables
GLFWwindow* window;
const int BASE_WINDOW_WIDTH = 1600, BASE_WINDOW_HEIGHT = 900;
int windowWidth = BASE_WINDOW_WIDTH, windowHeight = BASE_WINDOW_HEIGHT;
mat4 Mvp(1.0f), Mv(1.0f), Mp(1.0f);
bool isFullscreen = false, showMenu = true, tabPressed = false;
bool isMenuHovered = false;
bool enableDebugMode = false;
bool captureScreen = false;

// Object
GBO gbo(windowWidth, windowHeight);
int displayType = 5;
// Please ensure applying the matrices in STR order (R * T * S), or else you'd think
// you're bad at OpenGL, because the trice won't fuckin MOVE
// Focus
vec3 objectPos(3.0f, 1.0f, -1.5f), moveVec(0.0f), forwardVec(0.0f), rightVec(0.0f);
GLfloat baseMoveStep = 0.01f, runFactor = 1.5;
GLfloat moveStep = baseMoveStep;
bool isMovingW = false, isMovingS = false, isMovingA = false, isMovingD = false, isMovingUp = false, isMovingDown = false;
bool isMoving = false;
// Trice
vec3 tricePos(2.05, 0.628725, -1.9);
float triceScale = 0.001f;
mat4 MtriceS = scale(mat4(1.0f), vec3(triceScale));
mat4 MtriceT = translate(mat4(1.0f), tricePos);
mat4 Mtrice = MtriceT * MtriceS;

// Camera
float fov = radians(60.0f);
float aspectRatio = (float)windowWidth / (float)windowHeight;
float zNear = 0.1f, zFar = 10.0f;
const vec3 UP_VEC(0.0f, 1.0f, 0.0f);
// Distance (R), Theta (θ), Phi (φ), Focus; FoV, Aspect Ratio, Near, Far, Up vector
Camera camera(1.0f, 90.0f, 0.0f, vec3(0.0f), fov, aspectRatio, zNear, zFar, UP_VEC);
Mouse mouse;

// Lighting
vec3 lightPos(-2.845, 2.028, -1.293);
// Directional Shadow Mapping
const vec3 DSM_TARGET(0.542, -0.141, -0.422);
const float DSM_RANGE = 5.0f, DSM_NEAR = 0.1f, DSM_FAR = 10.0f;
mat4 MDSMp = ortho(DSM_RANGE / -2, DSM_RANGE / 2, DSM_RANGE / -2, DSM_RANGE / 2, DSM_NEAR, DSM_FAR);
mat4 MDSMv(1.0f), MDSM(1.0f);
// P, A, V Lights
enum class LightType {
    LightType_Point = 0,
    LightType_Area = 1,
    LightType_Volume = 2,
};
struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
};
struct AreaLight {
    vec3 position;
    vec3 diffuse;
    float size;
    float intensity;
};
struct VolumeLight {
    vec3 position;
};
// Point Light
PointLight pointLight = {
    vec3(1.87659, 0.4625, 0.103928), 1.0, 0.7, 0.14
};
float pLightScale = 0.22f;
mat4 MplS = scale(mat4(1.0f), vec3(pLightScale));
mat4 MplT(1.0f), Mpl(1.0f);
bool enablePL = true;
// Point Light Shadow
bool enablePLS = true;
const float PLS_NEAR = 0.22f, PLS_FAR = 10.0f;
mat4 MPLSp = perspective(radians(90.0f), (float)(1024 / 1024), PLS_NEAR, PLS_FAR);
vector<mat4> MPLS;
float plSampleRadius = 0.05, plShadowBias = 0.15;
int plSamples = 20;
// Area Light
AreaLight areaLight = {
    vec3(1.0, 0.5, -0.5), vec3(0.8, 0.6, 0.0), 1, 0.4
};
mat4 MalS = scale(mat4(1.0f), vec3(1.0f, 1.0f, 0.001f));
mat4 MalT(1.0f), Mal(1.0f);
bool enableAL = true;
// Volume Light
VolumeLight volumeLight = {
    vec3(-2.845 * 5, 2.028 * 2.5, -1.293 * 5)
};
float vLightScale = 1.5f;
mat4 MvlS = scale(mat4(1.0f), vec3(vLightScale));
mat4 MvlT(1.0f), Mvl(1.0f);
bool enableVL = true;

// Shaders
GLuint rectVAO, rectVBO, currentTexture;
FBO sceneFBO(windowWidth, windowHeight, FBOType::FBOType_2D);
bool useNormalMap = true;
// Blinn-Phong
bool enableBP = true;
// Directional Shadow Mapping
FBO dsmFBO(1024, 1024, FBOType::FBOType_Depth);
bool enableDSM = true;
int sampleRadius = 6;
float shadowBias = 0.002f, biasVariation = 0.01f;
// Point Light Shadow
FBO plFBO(windowWidth, windowHeight, FBOType::FBOType_2D);
FBO plsFBO(1024, 1024, FBOType::FBOType_DepthCube);
// Bloom
FBO brightPassFBO(windowWidth, windowHeight, FBOType::FBOType_2D);
FBO gaussianHFBO(windowWidth, windowHeight, FBOType::FBOType_2D);
FBO gaussianVFBO(windowWidth, windowHeight, FBOType::FBOType_2D);
FBO bloomFBO(windowWidth, windowHeight, FBOType::FBOType_2D);
float brightnessThreshold = 0.55, strength = 4.0;
// NPR
FBO edgeFBO(windowWidth, windowHeight, FBOType::FBOType_2D);
bool enableNPR = true;
float edgeThreshold = 0.8;
// FXAA
FBO fxaaFBO(windowWidth, windowHeight, FBOType::FBOType_2D);
bool enableFXAA = true;
const char* FXAA_LEVELS[] = { "3", "4", "5" };
const char* fxaaLevel = FXAA_LEVELS[2];
// SSAO
bool enableSSAO = false;
std::vector<glm::vec3> ssaoKernel;

// Screenshot
const char* SS_FORMATS[] = { "JPEG", "PNG" };
const char* ssFormat = SS_FORMATS[0];
bool ignoreAlpha = false;

float randomFloat() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.0, 1.0);

    return dis(gen);
}

/* -------------------------- CALLBACK -------------------------- */
static void errorCallback(int error, const char* description) {
    cerr << "ERROR: " << description << endl;
}

static void updateMovements() {
    vec3 newMoveVec(0);
    static vec3 lastHorizontalVec(0);

    if (isMovingW) newMoveVec += camera.forwardVec;
    if (isMovingS) newMoveVec -= camera.forwardVec;
    if (isMovingA) newMoveVec -= camera.rightVec;
    if (isMovingD) newMoveVec += camera.rightVec;

    bool isMovingHorizontally = isMovingW || isMovingS || isMovingA || isMovingD;
    if (isMovingHorizontally) lastHorizontalVec = newMoveVec;

    if (isMovingUp) newMoveVec += camera.getUpVector();
    if (isMovingDown) newMoveVec -= camera.getUpVector();

    if (length(newMoveVec) > 0) {
        moveVec = normalize(newMoveVec);
    } else {
        moveVec = vec3(0);
    }
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        tabPressed = !tabPressed;
        camera.enableMotion = !tabPressed;
        if (tabPressed) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    } else if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        showMenu = !showMenu;
    } else if (key == GLFW_KEY_F10 && action == GLFW_PRESS) {
        captureScreen = !captureScreen;
    }
    // TODO: Function broken
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        const GLFWvidmode* vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        if (isFullscreen) {
            glfwSetWindowMonitor(window, nullptr,
                                 (vidMode -> width - BASE_WINDOW_WIDTH) / 2,
                                 (vidMode -> height - BASE_WINDOW_HEIGHT) / 2,
                                  BASE_WINDOW_WIDTH, BASE_WINDOW_HEIGHT, GLFW_DONT_CARE);
        } else {
            glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0,
                                 vidMode -> width, vidMode -> height, GLFW_DONT_CARE);
        }
        isFullscreen = !isFullscreen;
    }
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_W:
                isMovingW = true;
                break;
            case GLFW_KEY_S:
                isMovingS = true;
                break;
            case GLFW_KEY_A:
                isMovingA = true;
                break;
            case GLFW_KEY_D:
                isMovingD = true;
                break;
            case GLFW_KEY_SPACE:
                isMovingUp = true;
                break;
            case GLFW_KEY_LEFT_CONTROL:
                isMovingDown = true;
                break;
        }
        updateMovements();
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_W:
            isMovingW = false;
            break;
        case GLFW_KEY_S:
            isMovingS = false;
            break;
        case GLFW_KEY_A:
            isMovingA = false;
            break;
        case GLFW_KEY_D:
            isMovingD = false;
            break;
        case GLFW_KEY_SPACE:
            isMovingUp = false;
            break;
        case GLFW_KEY_LEFT_CONTROL:
            isMovingDown = false;
            break;
        }
        updateMovements();
    }

    isMoving = isMovingW || isMovingS || isMovingA || isMovingD || isMovingUp || isMovingDown;
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (camera.enableMotion) {
        float limitAng = 22.5f;
        float deltaX = (xpos - mouse.lastCursorPos.x) / 100;
        float deltaY = (ypos - mouse.lastCursorPos.y) / 100;
        int dir = static_cast<int>(mouse.ctrlDir);

        float newTheta = std::clamp(camera.getTheta() - dir * mouse.yStep * deltaY, limitAng, 180.0f - limitAng);
        float newPhi = fmod((camera.getPhi() + dir * mouse.xStep * deltaX), 360);
        newPhi = (newPhi < 0) ? newPhi += 360.0f : newPhi;
        camera.setPhi(newPhi);
        camera.setTheta(newTheta);
    }
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    float factor = 1.0;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        factor = runFactor;
    }

    moveStep = factor * baseMoveStep;
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (!isMenuHovered || camera.enableMotion) {
        if (yoffset > 0)
            camera.zoomIn();
        else if (yoffset < 0)
            camera.zoomOut();
    }
}

static void frameBufferSizeCallback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    aspectRatio = (float)windowWidth / (float)windowHeight;
    camera.setAspectRatio(aspectRatio);
    glViewport(0, 0, width, height);

    // FBO resize
    gbo.resize(width, height);
    sceneFBO.resize(width, height);

    // Bloom
    plFBO.resize(width, height);
    brightPassFBO.resize(width, height);
    gaussianHFBO.resize(width, height);
    gaussianVFBO.resize(width, height);
    bloomFBO.resize(width, height);

    // NPR
    edgeFBO.resize(width, height);
    
    // FXAA
    fxaaFBO.resize(width, height);
}



/* --------------------------- RENDER --------------------------- */
static void flipImageVertically(unsigned char* pixels, const size_t width, const size_t height, const size_t channels) {
    const size_t stride = width * channels;
    unsigned char* row = (unsigned char*)malloc(stride);
    unsigned char* low = pixels;
    unsigned char* high = &pixels[(height - 1) * stride];

    for (; low < high; low += stride, high -= stride) {
        memcpy(row, low, stride);
        memcpy(low, high, stride);
        memcpy(high, row, stride);
    }
    free(row);
}

static void updateMVP() {
    // Camera
    Mp = camera.projMatrix();
    Mv = camera.viewMatrix();
    Mvp = Mp * Mv;

    // Point Light
    MplT = translate(mat4(1.0f), pointLight.position);
    Mpl = MplT * MplS;

    // Area Light
    MalT = translate(mat4(1.0f), areaLight.position);
    Mal = MalT * MalS;

    // Volume Light
    MvlT = translate(mat4(1.0f), volumeLight.position);
    Mvl = MvlT * MvlS;

    // Lighting
    // Directional Shadow Mapping
    MDSMv = lookAt(lightPos, DSM_TARGET, UP_VEC);
    MDSM = MDSMp * MDSMv;

    // Point Light Shadow
    MPLS.push_back(MPLSp * lookAt(pointLight.position, pointLight.position + vec3( 1.0, 0.0, 0.0), vec3(0.0,-1.0, 0.0)));
    MPLS.push_back(MPLSp * lookAt(pointLight.position, pointLight.position + vec3(-1.0, 0.0, 0.0), vec3(0.0,-1.0, 0.0)));
    MPLS.push_back(MPLSp * lookAt(pointLight.position, pointLight.position + vec3( 0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)));
    MPLS.push_back(MPLSp * lookAt(pointLight.position, pointLight.position + vec3( 0.0,-1.0, 0.0), vec3(0.0, 0.0,-1.0)));
    MPLS.push_back(MPLSp * lookAt(pointLight.position, pointLight.position + vec3( 0.0, 0.0, 1.0), vec3(0.0,-1.0, 0.0)));
    MPLS.push_back(MPLSp * lookAt(pointLight.position, pointLight.position + vec3( 0.0, 0.0,-1.0), vec3(0.0,-1.0, 0.0)));
}

static void updateObjectPosition() {
    if (isMoving) {
        objectPos += moveVec * moveStep;
    }
}

static void renderModel(const Model& model, Shader& shader, const mat4& mMat, const mat4& vMat, const mat4& pMat) {
    // MVP
    shader.setMat4("MM", mMat);  // Common
    shader.setMat4("MV", vMat);  // Common
    shader.setMat4("MP", pMat);  // Common

    // Lighting
    shader.setMat4("MDSM", MDSM);
    shader.setVec3("lightPos", lightPos);
    // Directional Shadow Mapping
    shader.setInt("sampleRadius", sampleRadius);
    shader.setFloat("shadowBias", shadowBias);
    shader.setFloat("biasVariation", biasVariation);

    // Options
    shader.setInt("useNM", useNormalMap);  // Common
    shader.setInt("enableBP", enableBP);
    shader.setInt("enableDSM", enableDSM);
    shader.setInt("enableNPR", enableNPR);

    model.render(shader);
}

static void renderScene(const Model& MODEL_ROOM, const Model& MODEL_TRICE,
                        const Model& MODEL_SPHERE, const Model& MODEL_CUBE, 
                        Shader& shader, GLuint shadowMap) {
    shader.activate();
    shader.setTexture2D("shadowMap", shadowMap, 6);  // For modelSP

    renderModel(MODEL_ROOM, shader, mat4(1.0f), Mv, Mp);
    renderModel(MODEL_TRICE, shader, Mtrice, Mv, Mp);
    renderModel(MODEL_CUBE, shader, Mal, Mv, Mp);
    renderModel(MODEL_SPHERE, shader, Mvl, Mv, Mp);
}

static void renderLight(const Model& MODEL_LIGHT, Shader& shader, LightType type) {
    shader.activate();

    // MVP
    shader.setMat4("MM", Mpl);
    shader.setMat4("MV", Mv);
    shader.setMat4("MP", Mp);

    // Options
    shader.setInt("isLight", true);
    shader.setInt("lightType", static_cast<int>(type));

    MODEL_LIGHT.render(shader);
}

static void renderFullScreenQuad(GLuint texture) {
    glBindVertexArray(rectVAO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

static void renderFullScreenQuad() {
    glBindVertexArray(rectVAO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}



/* --------------------------- FILTER --------------------------- */
static GLuint lightPass(GBO& sourceGBO, FBO& targetFBO, Shader& shader, GLuint dShadowMap, GLuint plShadowMap) {
    // Lighting Pass
    targetFBO.bind();
    shader.activate();
    shader.setTexture2D("gPosition", sourceGBO.getTexture(GBO_TEXTURE_TYPE_VERTEX), 0);
    shader.setTexture2D("gNormal", sourceGBO.getTexture(GBO_TEXTURE_TYPE_NORMAL), 1);
    shader.setTexture2D("gDiffuse", sourceGBO.getTexture(GBO_TEXTURE_TYPE_DIFFUSE), 2);
    shader.setTexture2D("gAmbient", sourceGBO.getTexture(GBO_TEXTURE_TYPE_AMBIENT), 3);
    shader.setTexture2D("gSpecular", sourceGBO.getTexture(GBO_TEXTURE_TYPE_SPECULAR), 4);
    shader.setTexture2D("gDepth", sourceGBO.getTexture(GBO_TEXTURE_TYPE_DEPTH), 5);
    shader.setTexture2D("shadowMap", dShadowMap, 6);
    shader.setTextureCube("plShadowMap", plShadowMap, 7);

    // MVP
    shader.setMat4("MV", Mv);
    shader.setMat4("MP", Mp);
    shader.setVec3("viewPos", camera.getPosition());

    // Lighting
    shader.setMat4("MDSM", MDSM);
    shader.setVec3("lightPos", lightPos);

    // Directional Shadow Mapping
    shader.setInt("sampleRadius", sampleRadius);
    shader.setFloat("shadowBias", shadowBias);
    shader.setFloat("biasVariation", biasVariation);

    // Lights
    shader.setVec3("pointLight.position", pointLight.position);
    shader.setFloat("pointLight.constant", pointLight.constant);
    shader.setFloat("pointLight.linear", pointLight.linear);
    shader.setFloat("pointLight.quadratic", pointLight.quadratic);
    shader.setFloat("plsFarPlane", PLS_FAR);
    shader.setFloat("plSampleRadius", plSampleRadius);
    shader.setFloat("plShadowBias", plShadowBias);
    shader.setInt("plSamples", plSamples);

    shader.setVec3("areaLight.position", areaLight.position);
    shader.setVec3("areaLight.diffuse", areaLight.diffuse);
    shader.setFloat("areaLight.size", areaLight.size);
    shader.setFloat("areaLight.intensity", areaLight.intensity);

    // Options
    shader.setInt("displayType", displayType);
    shader.setInt("enableBP", enableBP);
    shader.setInt("enableDSM", enableDSM);
    shader.setInt("enableNPR", enableNPR);
    shader.setInt("enablePL", enablePL);
    shader.setInt("enablePLS", enablePLS);
    shader.setInt("enableAL", enableAL);
    shader.setInt("enableVL", enableVL);
    shader.setInt("enableSSAO", enableSSAO);

    renderFullScreenQuad();
    targetFBO.unbind();

    return targetFBO.getTexture();
}

static GLuint bloomFilter(Shader& brightPassShader, Shader& gaussianShader, Shader& bloomShader, GLuint sourceTex) {
    // Bright pass + Gaussian blur
    brightPassFBO.bind();
    brightPassShader.activate();
    brightPassShader.setInt("screenTex", 0);
    brightPassShader.setFloat("brightnessThreshold", brightnessThreshold);
    renderFullScreenQuad(sourceTex);
    brightPassFBO.unbind();


    gaussianShader.activate();
    gaussianShader.setInt("brightTex", 0);

    gaussianHFBO.bind();
    gaussianShader.setInt("isHorizontal", GL_TRUE);
    brightPassShader.setFloat("sigma", strength);
    renderFullScreenQuad(brightPassFBO.getTexture());
    gaussianHFBO.unbind();

    gaussianVFBO.bind();
    gaussianShader.setInt("isHorizontal", GL_FALSE);
    renderFullScreenQuad(gaussianHFBO.getTexture());
    gaussianVFBO.unbind();

    // Bloom
    bloomFBO.bind();
    bloomShader.activate();
    bloomShader.setTexture2D("originalSceneTex", sourceTex, 0);
    bloomShader.setTexture2D("blurredBrightTex", gaussianVFBO.getTexture(), 1);
    renderFullScreenQuad();
    bloomFBO.unbind();

    return bloomFBO.getTexture();
}

static GLuint npr(FBO& edgeFBO, Shader& nprShader, GLuint sourceTex) {
    // Cel is written in light pass shader,
    // due the need of N and L vectors.
    edgeFBO.bind();
    nprShader.activate();
    nprShader.setInt("currentTex", 0);
    nprShader.setFloat("threshold", edgeThreshold);
    renderFullScreenQuad(sourceTex);
    edgeFBO.unbind();

    return edgeFBO.getTexture();
}

static GLuint fxaa(FBO& fxaaFBO, Shader& fxaaShader, GLuint sourceTex) {
    int level = stoi(fxaaLevel);

    fxaaFBO.bind();
    fxaaShader.activate();
    fxaaShader.setInt("currentTex", 0);
    fxaaShader.setVec2("texSize", vec2(windowWidth, windowHeight));
    switch (level) {
        case 3:
            fxaaShader.setFloat("FXAA_EDGE_THRESHOLD_MIN", (1.0 / 16.0));
            fxaaShader.setInt("FXAA_SEARCH_STEPS", 16);
            break;
        case 4:
            fxaaShader.setFloat("FXAA_EDGE_THRESHOLD_MIN", (1.0 / 24.0));
            fxaaShader.setInt("FXAA_SEARCH_STEPS", 24);
            break;
        case 5:
            fxaaShader.setFloat("FXAA_EDGE_THRESHOLD_MIN", (1.0 / 24.0));
            fxaaShader.setInt("FXAA_SEARCH_STEPS", 32);
            break;
    }
    renderFullScreenQuad(sourceTex);
    fxaaFBO.unbind();

    return fxaaFBO.getTexture();
}



/* --------------------------- IMGUI ---------------------------- */
static void startImGuiFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

static void renderImGuiFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int samplingRate = 100;
static void renderFramerateWidget(ImGuiIO& io) {
    static float values[100] = {};
    static int valuesOffset = 0;
    static double refreshTime = 0.0;

    // Sample framerate
    double currentTime = ImGui::GetTime();
    if (currentTime - refreshTime > (1.0f / samplingRate)) {
        values[valuesOffset] = io.Framerate;
        valuesOffset = (valuesOffset + 1) % IM_ARRAYSIZE(values);
        refreshTime = currentTime;
    }

    // Average framerate
    float average = 0.0f;
    for (int n = 0; n < IM_ARRAYSIZE(values); n++)
        average += values[n];
    average /= (float)IM_ARRAYSIZE(values);

    // Widget
    char overlay[32];
    sprintf(overlay, "%.2f FPS (avg %.3f ms)", average, 1000.0f / average);
    ImGui::PlotLines("Framerate", values, IM_ARRAYSIZE(values), valuesOffset, overlay, 0.0f, 200.0f, ImVec2(0, 60.0f));
    ImGui::Text("Sampling Rate");
    ImGui::RadioButton("10", &samplingRate, 10); ImGui::SameLine();
    ImGui::RadioButton("50", &samplingRate, 50); ImGui::SameLine();
    ImGui::RadioButton("100", &samplingRate, 100); ImGui::SameLine();
    ImGui::RadioButton("200", &samplingRate, 200);
}

static void renderGeneralMenu() {
    // General
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("General", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    renderFramerateWidget(io);
    // ImGui::TextWrapped("%.2f FPS (average %.3f ms)", io.Framerate, 1000.0f / io.Framerate);
    ImGui::TextWrapped("Need some help"); ImGui::SameLine(); ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::LabelText("WASD", "Move");
        ImGui::LabelText("Space", "Ascend");
        ImGui::LabelText("L_CTRL", "Descend");
        ImGui::LabelText("R_PRESS", "Speed Up");
        ImGui::LabelText("Scroll", "Zoom");
        ImGui::LabelText("Tab", "Boundless View Control");
        ImGui::LabelText("M", "Menu Visibility");
        ImGui::LabelText("F", "Fullscreen");
        ImGui::LabelText("F10", "Screenshot");
        ImGui::LabelText("Esc", "Exit");
        ImGui::EndTooltip();
    }

    // Info
    ImGui::SeparatorText("Info");
    if (ImGui::BeginTable("infoTable", 5, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn("Space", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("B", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("C", ImGuiTableColumnFlags_WidthFixed, 50.0f);

        // Focus
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("XYZ");

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%.3f", objectPos.x);

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%.3f", objectPos.y);

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%.3f", objectPos.z);

        // Camera
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("RTP");

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%.2f", camera.getRealDistance());

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%.1f", camera.getTheta());

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%.1f", camera.getPhi());

        ImGui::EndTable();
    }


    ImGui::SliderFloat("Zoom Step", &camera.zoomStep, 0.05f, 0.5f, "%.2f");

    // Cursor Control
    ImGui::SeparatorText("Control");
    if (ImGui::BeginMenu("Control Direction")) {
        if (ImGui::MenuItem("Normal", nullptr, mouse.ctrlDir == Mouse::CtrlDir::NORMAL)) {
            mouse.ctrlDir = Mouse::CtrlDir::NORMAL;
        }
        if (ImGui::MenuItem("Reversed", nullptr, mouse.ctrlDir == Mouse::CtrlDir::REVERSED)) {
            mouse.ctrlDir = Mouse::CtrlDir::REVERSED;
        }
        ImGui::EndMenu();
    }
    ImGui::SliderFloat("Move Speed", &baseMoveStep, 0.01f, 0.2f, "%.2f");
    ImGui::SliderFloat("Hor. Sen.", &mouse.xStep, 1.0f, 10.0f, "%.1f");
    ImGui::SliderFloat("Ver. Sen.", &mouse.yStep, 1.0f, 10.0f, "%.1f");

    // Screenshot
    ImGui::SeparatorText("Screenshot");
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::BeginCombo("Output Format", ssFormat)) {
        for (int i = 0; i < IM_ARRAYSIZE(SS_FORMATS); i++) {
            const bool isSelected = (SS_FORMATS[i] == ssFormat);
            if (ImGui::Selectable(SS_FORMATS[i], isSelected))
                ssFormat = SS_FORMATS[i];
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ssFormat == "PNG") ImGui::Checkbox("Ignore alpha channel", &ignoreAlpha);

    ImGui::End();
}

static void renderLightShadeMenu() {
    ImGui::Begin("Lighting & Shading", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    // Lighting
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Lighting")) {
        // Lighting - Basic
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Basic")) {
            ImGui::DragFloat3("Position", (float*)&lightPos, 0.01f, -3.0f, 3.0f, "%.2f"); ImGui::SameLine(); ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Both Blinn-Phong and DSM would be affected.");
                ImGui::EndTooltip();
            }
            ImGui::Checkbox("Enable Blinn-Phong", &enableBP);

            ImGui::TreePop();
        }

        // Lighting - Directional Shadow Mapping
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Directional Shadow Mapping")) {
            ImGui::Checkbox("Enable DSM", &enableDSM);
            if (enableDSM) {
                ImGui::SliderInt("Sample Radius", &sampleRadius, 2, 8, "%d");
                ImGui::SliderFloat("Shadow Bias", &shadowBias, 0.001f, 0.02f, "%.3f");
                ImGui::SliderFloat("Bias Variation", &biasVariation, 0.01f, 0.05f, "%.3f");
            }

            ImGui::TreePop();
        }

        // Lighting - Light Types
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Point/Area/Volume Lights")) {
            // Light Types - Point
            ImGui::SeparatorText("Point Light");
            ImGui::Checkbox("Enable PL", &enablePL);
            if (!enablePL) enablePLS = false;
            if (enablePL) {
                ImGui::DragFloat3("PL Position", (float*)&pointLight.position, 0.01f, -3.0f, 3.0f, "%.2f");
                ImGui::Checkbox("Point Light Shadow", &enablePLS);
                if (enablePLS) {
                    ImGui::SliderFloat("Sample Radius", &plSampleRadius, 0.01f, 0.1f, "%.2f");
                    ImGui::SliderFloat("Shadow Bias", &plShadowBias, 0.01f, 0.2f, "%.3f");
                    ImGui::SliderInt("Samples", &plSamples, 4, 32, "%d");
                }
            }

            // Light Types - Area
            ImGui::SeparatorText("Area Light");
            ImGui::Checkbox("Enable AL", &enableAL);
            if (enableAL) {
                ImGui::DragFloat3("AL Position", (float*)&areaLight.position, 0.01f, -3.0f, 3.0f, "%.2f");
            }

            // Light Types - Volume
            ImGui::SeparatorText("Volume Light");
            ImGui::Checkbox("Enable VL", &enableVL);
            if (enableVL) {
                ImGui::DragFloat3("VL Position", (float*)&volumeLight.position, 0.01f, -20.0f, 20.0f, "%.2f");
            }

            ImGui::TreePop();
        }
    }

    // Shading
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Shading")) {
        // Shading - Normal Mapping
        ImGui::Checkbox("Enable Normal Map", &useNormalMap);

        // Shading - Deferred Shading
        if (ImGui::BeginCombo("G-buffer Texture", GBOTextureDisplayTypes[displayType])) {
            for (int i = 0; i < IM_ARRAYSIZE(GBOTextureDisplayTypes); i++) {
                const bool isSelected = (i == displayType);
                if (ImGui::Selectable(GBOTextureDisplayTypes[i], isSelected))
                    displayType = i;
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }
        ImGui::SameLine(); ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Options other than \"Result\" will skip every post-processing.");
            ImGui::EndTooltip();
        }

        // Shading - NPR
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("NPR")) {
            ImGui::Checkbox("Enable NPR", &enableNPR);
            if (enableNPR) {
                ImGui::SliderFloat("Edge Threshold", &edgeThreshold, 0.4f, 1.0f, "%.1f");
            }

            ImGui::TreePop();
        }

        // Shading - FXAA
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("FXAA")) {
            ImGui::Checkbox("Enable FXAA", &enableFXAA);
            if (enableFXAA) {
                if (ImGui::BeginCombo("Level", fxaaLevel)) {
                    for (int i = 0; i < IM_ARRAYSIZE(FXAA_LEVELS); i++) {
                        const bool isSelected = (FXAA_LEVELS[i] == fxaaLevel);
                        if (ImGui::Selectable(FXAA_LEVELS[i], isSelected))
                            fxaaLevel = FXAA_LEVELS[i];
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }
            }

            ImGui::TreePop();
        }
        ImGui::Checkbox("Enable SSAO", &enableSSAO);

    }

    ImGui::End();
}

static void closeImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}



/* ---------------------------- INIT ---------------------------- */
static void APIENTRY GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length,
    const GLchar* msg, const void* data) {
    const char* _source;
    const char* _type;
    const char* _severity;

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            _source = "API";
            break;

        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            _source = "WINDOW SYSTEM";
            break;

        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            _source = "SHADER COMPILER";
            break;

        case GL_DEBUG_SOURCE_THIRD_PARTY:
            _source = "THIRD PARTY";
            break;

        case GL_DEBUG_SOURCE_APPLICATION:
            _source = "APPLICATION";
            break;

        case GL_DEBUG_SOURCE_OTHER:
            _source = "UNKNOWN";
            break;

        default:
            _source = "UNKNOWN";
            break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            _type = "ERROR";
            break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            _type = "DEPRECATED BEHAVIOR";
            break;

        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            _type = "UNDEFINED BEHAVIOR";
            break;

        case GL_DEBUG_TYPE_PORTABILITY:
            _type = "PORTABILITY";
            break;

        case GL_DEBUG_TYPE_PERFORMANCE:
            _type = "PERFORMANCE";
            break;

        case GL_DEBUG_TYPE_OTHER:
            _type = "OTHER";
            break;

        case GL_DEBUG_TYPE_MARKER:
            _type = "MARKER";
            break;

        default:
            _type = "UNKNOWN";
            break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            _severity = "HIGH";
            break;

        case GL_DEBUG_SEVERITY_MEDIUM:
            _severity = "MEDIUM";
            break;

        case GL_DEBUG_SEVERITY_LOW:
            _severity = "LOW";
            break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
            _severity = "NOTIFICATION";
            break;

        default:
            _severity = "UNKNOWN";
            break;
    }

    if (_type != "OTHER" && _severity != "NOTIFICATION") {
        printf("%d: %s of %s severity, raised from %s: %s\n",
            id, _type, _severity, _source, msg);
    }
    /*printf("%d: %s of %s severity, raised from %s: %s\n",
        id, _type, _severity, _source, msg); */
}

static GLFWwindow* glfwInitializer(GLFWwindow* wd, int width, int height, const char* name) {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        throw runtime_error("Failed to initialize GLFW");
    }

    // MSAA
    glfwWindowHint(GLFW_SAMPLES, 4);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const GLFWvidmode* vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    wd = glfwCreateWindow(width, height, name, NULL, NULL);
    if (!wd) {
        glfwTerminate();
        throw runtime_error("Failed to create GLFW window");
    }
    glfwSetWindowPos(wd, (vidMode -> width - windowWidth) / 2, (vidMode -> height - windowHeight) / 2);

    glfwSetInputMode(wd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(wd, frameBufferSizeCallback);
    glfwSetKeyCallback(wd, keyCallback);
    glfwSetCursorPosCallback(wd, cursorPosCallback);
    glfwSetMouseButtonCallback(wd, mouseButtonCallback);
    glfwSetScrollCallback(wd, scrollCallback);
    glfwMakeContextCurrent(wd);
    return wd;
}

static void imGuiInit(GLFWwindow* wd) {
    // Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    float rounded = 12.0f;
    style.TabRounding = rounded / 2;
    style.FrameRounding = rounded;
    style.GrabRounding = rounded;
    style.WindowRounding = rounded / 2;
    style.PopupRounding = rounded;

    // Platform / Renderer backends
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui_ImplGlfw_InitForOpenGL(wd, true);
    ImGui_ImplOpenGL3_Init("#version 450");
}

static void fullScreenQuadInit() {
    float rectVertices[24] = {
        // Positions   // Texture Coords
        -1.0f,  1.0f,  0.0f, 1.0f, // Top-left
         1.0f,  1.0f,  1.0f, 1.0f, // Top-right
         1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right

         1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right
        -1.0f, -1.0f,  0.0f, 0.0f, // Bottom-left
        -1.0f,  1.0f,  0.0f, 1.0f, // Top-left
    };

    // Generate VAO and VBO
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);

    // Bind the VAO
    glBindVertexArray(rectVAO);

    // Bind the VBO, buffer the vertex data
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), &rectVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind the VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void init() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw runtime_error("Failed to initialize GLAD");
    }

    // Please double check the working directory, or else you'd think
    // you're bad at OpenGL, because the shaders won't fuckin load.
    cout << "WORKING_DIR: " << fs::current_path() << endl;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // Depth Test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // MSAA
    glEnable(GL_MULTISAMPLE);
    // Blend mode
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    fullScreenQuadInit();

    if (enableDebugMode) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(GLDebugMessageCallback, nullptr);
    }
}

static void screenshot() {
    auto t = time(nullptr);
    auto tm = *localtime(&t);
    int year2d = (tm.tm_year + 1900) % 100;
    ostringstream oss;
    oss << setw(2) << setfill('0') << year2d << "_";
    oss << put_time(&tm, "%m_%d-%H_%M_%S");
    auto str = oss.str();

    string filename = "OUTPUT_" + str;
    string file;

    GLubyte* pixels = new GLubyte[windowWidth * windowHeight * 4];
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    flipImageVertically(pixels, windowWidth, windowHeight, 4);

    bool saveSuccessed = false;
    if (ssFormat == "JPEG") {
        file = filename + ".jpg";
        if (stbi_write_jpg(file.c_str(), windowWidth, windowHeight, 4, pixels, windowWidth * 4))
            saveSuccessed = true;
    }
    else if (ssFormat == "PNG") {
        file = filename + ".png";
        if (stbi_write_png(file.c_str(), windowWidth, windowHeight, 4, pixels, windowWidth * 4))
            saveSuccessed = true;
    }
    else {
        cerr << "SCREENSHOT::ERROR: Failed to save image." << endl;
    }

    if (saveSuccessed) cout << "Saved image to '" << file << "'" << endl;
    delete[] pixels;
    captureScreen = false;
}

std::vector<glm::vec3> initSSAOKernel() {
    //std::uniform_real_distribution<GLfloat> randomFloat(0.0, 1.0); 
    //std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            randomFloat() * 2.0 - 1.0,
            randomFloat() * 2.0 - 1.0,
            randomFloat());
        sample = glm::normalize(sample);
        sample *= randomFloat();
        float scale = float(i) / 64.0;

        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }
    return ssaoKernel;
}

/* ---------------------------- MAIN ---------------------------- */
int main(void) {
    // Initialize GLFW / ImGui / GLAD
    try {
        window = glfwInitializer(window, windowWidth, windowHeight, "Indoor Scene Lighting");
        imGuiInit(window);
        init();
    } catch (const exception& e) {
        cerr << "INIT_ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    // Shaders
    Shader modelSP("FP/shaders/model.vert", "FP/shaders/model.frag");
    // Directional Shadow Mapping
    Shader dsmSP("FP/shaders/dsm.vert", "FP/shaders/dsm.frag");
    // Deferred Shading
    Shader geometrySP("FP/shaders/geometry.vert", "FP/shaders/geometry.frag");
    Shader lightingSP("FP/shaders/filter.vert", "FP/shaders/light.frag");
    // Point Light
    Shader brightPassSP("FP/shaders/filter.vert", "FP/shaders/bright_pass.frag");
    Shader gaussianSP("FP/shaders/filter.vert", "FP/shaders/gaussian_blur.frag");
    Shader bloomSP("FP/shaders/filter.vert", "FP/shaders/bloom.frag");
    // Point Light Shadow
    Shader plsSP("FP/shaders/pls.vert", "FP/shaders/pls.geom", "FP/shaders/pls.frag");
    // NPR
    Shader edgeSP("FP/shaders/filter.vert", "FP/shaders/edge_detection.frag");
    // FXAA
    Shader fxaaSP("FP/shaders/filter.vert", "FP/shaders/fxaa.frag");
    // Output
    Shader finalSP("FP/shaders/filter.vert", "FP/shaders/final.frag");

    // Models
    Model room("FP/models/Grey White Room.obj");
    Model trice("FP/models/Trice.obj");
    Model sphere("FP/models/Sphere.obj");
    Model cube("FP/models/Cube.obj");

    // GBO & FBOs
    gbo.init();
    sceneFBO.init();
    // Directional Shadow Mapping
    dsmFBO.init();
    // P, A, V Lights
    plFBO.init();
    plsFBO.init();
    // NPR
    edgeFBO.init();
    // FXAA
    fxaaFBO.init();
    
    // SSAO's kernel
    ssaoKernel = initSSAOKernel();



#ifdef VSYNC_DISABLED
    glfwSwapInterval(0);
#endif
    // Render loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ImGui
        if (showMenu) startImGuiFrame();

        // Update cursor position, camera position, MVP
        isMenuHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
        glfwGetCursorPos(window, &mouse.lastCursorPos.x, &mouse.lastCursorPos.y);
        updateObjectPosition();
        camera.focus = vec3(objectPos);
        camera.updatePosition();
        updateMVP();

        // Shadow map size
        glViewport(0, 0, 1024, 1024);

        // DSM
        {
            dsmFBO.bind();
            dsmSP.activate();

            dsmSP.setMat4("MshadowMap", MDSM);
            room.render(dsmSP);

            dsmSP.setMat4("MshadowMap", MDSM * Mtrice);
            trice.render(dsmSP);

            dsmFBO.unbind();
        }

        // PLS (does not consider light sources)
        {
            plsFBO.bind();
            plsSP.activate();

            plsSP.setMat4("MM", mat4(1.0f));
            plsSP.setMat4Array("MPLS", MPLS);
            plsSP.setVec3("lightPos", pointLight.position);
            plsSP.setFloat("farPlane", PLS_FAR);
            room.render(plsSP);

            plsSP.setMat4("MM", Mtrice);
            plsSP.setMat4Array("MPLS", MPLS);
            plsSP.setVec3("lightPos", pointLight.position);
            plsSP.setFloat("farPlane", PLS_FAR);
            trice.render(plsSP);

            plsFBO.unbind();
        }

        glViewport(0, 0, windowWidth, windowHeight);

#ifndef DEFERRED_SHADING
        // Point Light
        if (enablePL) {
            plFBO.bind();
            renderLight(sphere, modelSP, LightType::LightType_Point);
            plFBO.unbind();
        }

        // Render scene
        sceneFBO.bind();
        renderScene(room, trice, modelSP, dsmFBO.getTexture());
        sceneFBO.unbind();

        currentTexture = sceneFBO.getTexture();
#endif

#ifdef DEFERRED_SHADING
        // ...And another thing — do NOT forget to disable GL_BLEND.  — anossovp, 8 years ago
        // If I were find this comment a week ago, that would save a LOT of debugging  — Sandor Muranyi, 6 years ago
        // https://learnopengl.com/Advanced-Lighting/Deferred-Shading
        glDisable(GL_BLEND);

        // Geometry Pass
        gbo.bindWrite();
        renderScene(room, trice, sphere, cube, geometrySP, dsmFBO.getTexture());
        gbo.unbind();
        
        // Lighting Pass
        currentTexture = lightPass(gbo, sceneFBO, lightingSP, dsmFBO.getTexture(), plsFBO.getTexture());

        // Light Source Rendering
        /*plFBO.bindWrite();
        gbo.bindRead();
        glBlitFramebuffer(0, 0, gbo.getWidth(), gbo.getHeight(),
                          0, 0, gbo.getWidth(), gbo.getHeight(),
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        renderLight(sphere, modelSP, LightType::LightType_Point);
        plFBO.unbind();*/
#endif

        // FXAA, NPR
        if (displayType == 5) {
            if (enableNPR)
                currentTexture = npr(edgeFBO, edgeSP, currentTexture);
            if (enableFXAA)
                currentTexture = fxaa(fxaaFBO, fxaaSP, currentTexture);
        }

        // Final result
        finalSP.activate();
        finalSP.setInt("currentTex", 0);
        renderFullScreenQuad(currentTexture);
        // renderFullScreenQuad(plFBO.getTexture());
        


        // Screen capturing
        if (captureScreen)
            screenshot();

        // ImGui
        if (showMenu) {
            renderGeneralMenu();
            renderLightShadeMenu();
            renderImGuiFrame();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    closeImGui();
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
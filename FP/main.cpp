#include <iostream>
#include <filesystem>
#include <cmath>

// #define VSYNC_DISABLED
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
bool enableDebugMode = false;

// Object
vec3 objectPos(3.0f, 1.0f, -1.5f), moveVec(0.0f), forwardVec(0.0f), rightVec(0.0f);
GLfloat baseMoveStep = 0.01f, runFactor = 1.5;
GLfloat moveStep = baseMoveStep;
// Position & Rotation
bool isMovingW = false, isMovingS = false, isMovingA = false, isMovingD = false, isMovingUp = false, isMovingDown = false;
bool isMoving = false;

vec3 tricePos(2.05, 0.628725, -1.9);
float triceScale = 0.001f;
// Please ensure applying the matrices in STR order (R * T * S), or else you'd think
// you're bad at OpenGL, because the trice won't fuckin MOVE
mat4 MtriceS = scale(mat4(1.0f), vec3(triceScale));
mat4 MtriceT = translate(mat4(1.0f), tricePos);
mat4 Mtrice = MtriceT * MtriceS;

// Camera
float fov = radians(60.0f);
float aspectRatio = (float)windowWidth / (float)windowHeight;
float zNear = 0.01f, zFar = 150.0f;
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
int sampleRadius = 4;
float shadowBias = 0.025f, biasVariation = 0.03f;

// Shaders
GLuint rectVAO, rectVBO, currentTexture;
FBO sceneFBO(windowWidth, windowHeight, FBOType::FBOType_Normal);
bool captureScreen = false;
bool useNormalMap = true;
// Directional Shadow Mapping
FBO dsmFBO(1024, 1024, FBOType::FBOType_Depth);
// Cel Shading
FBO celFBO(windowWidth, windowHeight, FBOType::FBOType_Normal);
bool cel = false;



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
    if (yoffset > 0) {
        camera.zoomIn();
    }
    else if (yoffset < 0) {
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
    sceneFBO.resize(width, height);
    celFBO.resize(width, height);
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

    // Lighting
    MDSMv = lookAt(lightPos, DSM_TARGET, UP_VEC);
    MDSM = MDSMp * MDSMv;
}

static void updateObjectPosition() {
    if (isMoving) {
        objectPos += moveVec * moveStep;
    }
}

static void renderModel(const Model& model, Shader& shader, GLuint shadowMap, const mat4& mMat, const mat4& vMat, const mat4& pMat) {
    shader.setInt("useNormalMap", useNormalMap);

    // Matrices
    shader.setMat4("MM", mMat);
    shader.setMat4("MV", vMat);
    shader.setMat4("MP", pMat);

    // Lighting
    shader.setMat4("MDSM", MDSM);
    shader.setVec3("lightPos", lightPos);
    shader.setInt("sampleRadius", sampleRadius);
    shader.setFloat("shadowBias", shadowBias);
    shader.setFloat("biasVariation", biasVariation);
    
    
    model.render(shader);
}

static void renderScene(const Model& MODEL_ROOM, const Model& MODEL_TRICE, Shader& shader, GLuint shadowMap) {
    shader.activate();
    shader.setTexture("shadowMap", shadowMap, 2);

    renderModel(MODEL_ROOM, shader, shadowMap, mat4(1.0f), Mv, Mp);
    renderModel(MODEL_TRICE, shader, shadowMap, Mtrice, Mv, Mp);
}

static void renderSceneDSM(const Model& MODEL_ROOM, const Model& MODEL_TRICE, Shader& shader) {
    shader.activate();

    shader.setMat4("MDSM", MDSM);
    MODEL_ROOM.render(shader);

    shader.setMat4("MDSM", MDSM * Mtrice);
    MODEL_TRICE.render(shader);
}

static void renderFullScreenQuad(GLuint texture) {
    glBindVertexArray(rectVAO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void celShading() {
    currentTexture = celFBO.getTexture();
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
    ImGui::Text("Sampling Rate:");
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

    ImGui::End();
}

static void renderLightingMenu() {
    ImGui::Begin("Lighting & Shading", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    // Lighting
    ImGui::SeparatorText("Lighting");
    ImGui::DragFloat3("Light Pos.", (float*)&lightPos, 0.01f, -3.0f, 3.0f, "%.2f");

    // Lighting - Directional Shadow Mapping
    if (ImGui::CollapsingHeader("Directional Shadow Mapping")) {
        ImGui::SliderInt("Sample Radius", &sampleRadius, 2, 8, "%d");
        ImGui::SliderFloat("Shadow Bias", &shadowBias, 0.005f, 0.05f, "%.3f");
        ImGui::SliderFloat("Bias Variation", &biasVariation, 0.01f, 0.05f, "%.3f");
    }

    ImGui::SeparatorText("Shading");
    ImGui::Checkbox("Enable Normal Map", &useNormalMap);

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
            _type = "UDEFINED BEHAVIOR";
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
    style.TabRounding = rounded;
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
    Shader celSP("FP/shaders/filter.vert", "FP/shaders/cel.frag");
    Shader dsmSP("FP/shaders/dsm.vert", "FP/shaders/dsm.frag");
    Shader finalSP("FP/shaders/filter.vert", "FP/shaders/final.frag");

    // Models
    Model room("FP/models/Grey White Room.obj");
    Model trice("FP/models/Trice.obj");

    // FBOs
    sceneFBO.init();
    dsmFBO.init();
    celFBO.init();



#ifdef VSYNC_DISABLED
    glfwSwapInterval(0);
#endif
    // Render loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ImGui
        if (showMenu) startImGuiFrame();

        // Update cursor position, camera position, MVP
        glfwGetCursorPos(window, &mouse.lastCursorPos.x, &mouse.lastCursorPos.y);
        updateObjectPosition();
        camera.focus = vec3(objectPos);
        camera.updatePosition();
        updateMVP();

        // DSM
        dsmFBO.bind();
        dsmSP.activate();
        renderSceneDSM(room, trice, dsmSP);
        dsmFBO.unbind();

        // Render whole scene
        sceneFBO.bind();
        modelSP.activate();
        renderScene(room, trice, modelSP, dsmFBO.getTexture());
        sceneFBO.unbind();

        currentTexture = sceneFBO.getTexture();

        // Render final result to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        finalSP.activate();
        finalSP.setInt("tex", 0);
        renderFullScreenQuad(currentTexture);
        


        // Screen capturing
        if (captureScreen) {
            GLubyte* pixels = new GLubyte[windowWidth * windowHeight * 4];
            glReadPixels(0, 0, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            flipImageVertically(pixels, windowWidth, windowHeight, 4);

            if (stbi_write_png("output.png", windowWidth, windowHeight, 4, pixels, windowWidth * 4)) {
                cout << "Saved image to 'output.png'" << endl;
            }
            else {
                cerr << "Failed to save image." << endl;
            }
            delete[] pixels;
            captureScreen = false;
        }

        // ImGui
        if (showMenu) {
            renderGeneralMenu();
            renderLightingMenu();
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
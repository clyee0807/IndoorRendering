#include <iostream>
#include <filesystem>

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
int windowWidth = 1600, windowHeight = 900;
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

// Camera
float fov = radians(60.0f);
float aspectRatio = (float)windowWidth / (float)windowHeight;
float zNear = 0.01f, zFar = 150.0f;
const vec3 UP_VEC(0.0f, 1.0f, 0.0f);
// Distance (R), Theta (θ), Phi (φ), Focus; FoV, Aspect Ratio, Near, Far, Up vector
Camera camera(1.0f, 90.0f, 0.0f, vec3(0.0f), fov, aspectRatio, zNear, zFar, UP_VEC);
Mouse mouse;

// Shaders
GLuint rectVAO, rectVBO;
GLuint currentTexture;
FBO sceneFBO;
bool captureScreen = false;
bool useNormalMap = true;
// Cel Shading
FBO celFBO;
bool cel = false;



/* -------------------------- CALLBACK -------------------------- */
void errorCallback(int error, const char* description) {
    cerr << "ERROR: " << description << endl;
}

void updateMovements() {
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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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
                                 (vidMode -> width - 1280) / 2,
                                 (vidMode -> height - 960) / 2,
                                 1280, 960, GLFW_DONT_CARE);
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

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (camera.enableMotion) {
        float limitAng = 22.5f;
        float deltaX = (xpos - mouse.lastCursorPos.x) / 100;
        float deltaY = (ypos - mouse.lastCursorPos.y) / 100;
        int dir = static_cast<int>(mouse.ctrlDir);
        camera.setPhi(camera.getPhi() + dir * mouse.xStep * deltaX);
        camera.setTheta(std::clamp(camera.getTheta() - dir * mouse.yStep * deltaY,
                        limitAng,
                        180.0f - limitAng));
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    float factor = 1.0;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        factor = runFactor;
    }

    moveStep = factor * baseMoveStep;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (yoffset > 0) {
        camera.zoomIn();
    }
    else if (yoffset < 0) {
        camera.zoomOut();
    }
}

void frameBufferSizeCallback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);

    // FBO resize
    sceneFBO.resize(width, height);
    celFBO.resize(width, height);
}



/* --------------------------- RENDER --------------------------- */
void flipImageVertically(unsigned char* pixels, const size_t width, const size_t height, const size_t channels) {
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

void updateMVP() {
    Mp = camera.projMatrix();
    Mv = camera.viewMatrix();
    Mvp = Mp * Mv;
}

void updateObjectPosition() {
    if (isMoving) {
        objectPos += moveVec * moveStep;
    }
}

void renderModel(const Model& model, Shader& shader, const mat4& mvMat, const mat4& vMat, const mat4& pMat) {
    shader.activate();

    GLint useNM = glGetUniformLocation(shader.ID, "useNormalMap");
    glUniform1i(useNM, useNormalMap ? 1 : 0);

    GLint MMV = glGetUniformLocation(shader.ID, "MMV");
    glUniformMatrix4fv(MMV, 1, GL_FALSE, value_ptr(mvMat));
    GLint MV = glGetUniformLocation(shader.ID, "MV");
    glUniformMatrix4fv(MV, 1, GL_FALSE, value_ptr(vMat));
    GLint MP = glGetUniformLocation(shader.ID, "MP");
    glUniformMatrix4fv(MP, 1, GL_FALSE, value_ptr(pMat));
    
    model.render(shader);
}

void renderFullScreenQuad() {
    glBindVertexArray(rectVAO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void renderFullScreenQuad(GLuint texture) {
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
void startImGuiFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void renderImGuiFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void renderGeneralMenu() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("General", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Average %.3f ms / frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Separator();

    // ImGui::Text("[W][A][S][D] to move, [SPACE][LCTRL] to move up or down\n");
    // ImGui::Text("RIGHT PRESS to speed up the character\n");
    // ImGui::Text("SCROLL up / down to zoom in or out");
    // ImGui::Text("Cursor movement rotates the view around the character (boundlessly), [TAB] to toggle this behaviour\n");
    // ImGui::Text("[M] to toggle the visibility of this menu");
    // ImGui::Text("[F] to toggle fullscreen mode");
    // ImGui::Text("[F10] to screenshot");
    // ImGui::Text("[ESC] to exit");
    // ImGui::Separator();

    ImGui::Text("XYZR: %.1f, %.1f, %.1f, %.2f°", objectPos.x, objectPos.y, objectPos.z);
    ImGui::Text("RTP: %.2f, %.1f°, %.1f°", camera.getRealDistance(), camera.getTheta(), camera.getPhi());
    ImGui::SliderFloat("Zoom Step", &camera.zoomStep, 0.05f, 0.5f, "%.2f");
    ImGui::Separator();

    // Control Direction
    ImGui::SliderFloat("Move Speed", &baseMoveStep, 0.01f, 0.2f, "%.2f");
    ImGui::SliderFloat("Hor. Sen.", &mouse.xStep, 1.0f, 10.0f, "%.1f");
    ImGui::SliderFloat("Ver. Sen.", &mouse.yStep, 1.0f, 10.0f, "%.1f");
    if (ImGui::BeginMenu("Control Direction")) {
        if (ImGui::MenuItem("Normal", nullptr, mouse.ctrlDir == Mouse::CtrlDir::NORMAL)) {
            mouse.ctrlDir = Mouse::CtrlDir::NORMAL;
        }
        if (ImGui::MenuItem("Reversed", nullptr, mouse.ctrlDir == Mouse::CtrlDir::REVERSED)) {
            mouse.ctrlDir = Mouse::CtrlDir::REVERSED;
        }
        ImGui::EndMenu();
    }
    ImGui::Separator();

    //
    ImGui::Checkbox("Enable Normal Map", &useNormalMap);

    ImGui::End();
}

void closeImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}



/* ---------------------------- INIT ---------------------------- */
void APIENTRY GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
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

    printf("%d: %s of %s severity, raised from %s: %s\n",
        id, _type, _severity, _source, msg);
}

GLFWwindow* glfwInitializer(GLFWwindow* wd, int width, int height, const char* name) {
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

void imGuiInit(GLFWwindow* wd) {
    // Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    float rounded = 6.0f;
    style.TabRounding = rounded;
    style.FrameRounding = rounded;
    style.GrabRounding = rounded;
    style.WindowRounding = rounded;
    style.PopupRounding = rounded;

    // Platform / Renderer backends
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui_ImplGlfw_InitForOpenGL(wd, true);
    ImGui_ImplOpenGL3_Init("#version 450");
}

void fullScreenQuadInit() {
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

void init() {
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
        window = glfwInitializer(window, windowWidth, windowHeight, "GPU Driven Rendering");
        imGuiInit(window);
        init();
    } catch (const exception& e) {
        cerr << "INIT_ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    
    fullScreenQuadInit();
    
    GLint originalFrameBuffer;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &originalFrameBuffer);
    
#pragma region GBuffer Initialization
    GLuint gBuffer;
    glGenFramebuffers(1, &gBuffer);
    //glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    GLuint gPosition, gNormal, gAlbedo, gSpecular;
    /*
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gNormal, 0);
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gAlbedo, 0);
    glGenTextures(1, &gSpecular);
    glBindTexture(GL_TEXTURE_2D, gSpecular);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gSpecular, 0);
    
    //depth texture
    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, windowWidth, windowHeight, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthMap, 0);
    
    GLenum drawBuffers1[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, drawBuffers1);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << std::hex << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
        return -1;
    }
#pragma endregion
*/
/*
#pragma region lighting pass Buffer Initialization
    GLuint LPFB; //Lighting Pass Buffer
    glGenFramebuffers(1, &LPFB);
    glBindFramebuffer(GL_FRAMEBUFFER, LPFB);
    GLuint colorBuffer;
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    GLenum drawBuffers2[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers2);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "problems binding LPFB" << std::endl;
        return -1;
    }
#pragma endregion

#pragma region SSR pass Buffer Initialization
    GLuint ssrFB; //SSR Frame Buffer
    glGenFramebuffers(1, &ssrFB);
    glBindFramebuffer(GL_FRAMEBUFFER, ssrFB);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthMap, 0);
    GLuint reflectionColorBuffer;
    glGenTextures(1, &reflectionColorBuffer);
    glBindTexture(GL_TEXTURE_2D, reflectionColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionColorBuffer, 0);
    GLenum drawBuffers3[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers3);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "problems binding ssrFB" << std::endl;
        return -1;
    }
#pragma endregion
*/
    
    // Shaders
    Shader modelSP("FP/shaders/model.vert", "FP/shaders/model.frag");
    Shader celSP("FP/shaders/filter.vert", "FP/shaders/cel.frag");
    Shader finalSP("FP/shaders/filter.vert", "FP/shaders/final.frag");
    
    Shader lightingPassShader("FP/shaders/deferred.vert", "FP/shaders/deferred.frag");
    lightingPassShader.activate();
    lightingPassShader.setInt("gNormal", 0);
    lightingPassShader.setInt("gAlbedo", 1);
    lightingPassShader.setInt("gSpecular", 2);
    lightingPassShader.setInt("depthMap", 3);
    glm::vec3 lights[] = {
        glm::vec3(20, 10, 10), glm::vec3(1,1,1), glm::vec3(1,0.007f,0.0002f),          //position, color, (constant, linear, quadratic)
        glm::vec3(-25, -5, -35), glm::vec3(1,1,1), glm::vec3(1,0.007f,0.0002f),
        glm::vec3(25, -5, -35), glm::vec3(1,1,1), glm::vec3(1,0.027,0.0028),
    };
    for (int i = 0; i < sizeof(lights) / sizeof(lights[0]); i++)
    {
        lightingPassShader.setVec3("lights[" + std::to_string(i) + "].intensity", lights[i * 3 + 1]);
        lightingPassShader.setFloat("lights[" + std::to_string(i) + "].constant", lights[i * 3 + 2].x);
        lightingPassShader.setFloat("lights[" + std::to_string(i) + "].linear", lights[i * 3 + 2].y);
        lightingPassShader.setFloat("lights[" + std::to_string(i) + "].quadratic", lights[i * 3 + 2].z);
    }
    
    Shader SSRShader("FP/shaders/ssr.vert", "FP/shaders/ssr.frag");
    SSRShader.activate();
    SSRShader.setInt("gNormal", 0);
    SSRShader.setInt("colorBuffer", 1);
    SSRShader.setInt("depthMap", 2);
    Shader outputShader("FP/shaders/ssr.vert", "FP/shaders/output.frag");
    outputShader.activate();
    outputShader.setInt("colorTexture", 0);
    outputShader.setInt("refTexture", 1);
    
    // Models
    //cout << glGetError() << endl;
    Model room("FP/models/Grey White Room.obj");
    //cout << glGetError() << endl;
    Model trice("FP/models/Trice.obj");



    // Render loop
    while (!glfwWindowShouldClose(window)) {
        //Forward Pass
        //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer);
        glClearColor(1.0, 1.0, 1.0, 1.0f);
        //glStencilMask(0xFF);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_EQUAL, 0, 0xFF);
        glStencilMask(0x00);

        // ImGui
        if (showMenu) startImGuiFrame();

        // Update cursor position, camera position, MVP
        glfwGetCursorPos(window, &mouse.lastCursorPos.x, &mouse.lastCursorPos.y);
        updateObjectPosition();
        camera.focus = vec3(objectPos);
        camera.updatePosition();
        updateMVP();

        // Render whole scene
        // sceneFBO.bind();
        modelSP.activate();
        renderModel(room, modelSP, Mv, Mv, Mp);
        // Please ensure applying the matrices in STR order (R * T * S), or else you'd think
        // you're bad at OpenGL, because the trice won't fuckin MOVE
        mat4 MtriceS = scale(mat4(1.0f), vec3(triceScale));
        mat4 MtriceT = translate(mat4(1.0f), tricePos);
        mat4 Mtrice = MtriceT * MtriceS;
        renderModel(trice, modelSP, Mv * Mtrice, Mv, Mp);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        /*
        sceneFBO.unbind();
        currentTexture = sceneFBO.getTexture();
        // Split by separator
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        finalSP.activate();
        glUniform1i(glGetUniformLocation(finalSP.ID, "tex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        renderFullScreenQuad();
        */
        /*
        //Deferred(Lighting) Pass
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, originalFrameBuffer);
        glClearColor(1.0, 1.0, 1.0, 1.0f);
        //glClear(GL_COLOR_BUFFER_BIT);
        glStencilMask(0xFF);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gSpecular);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        lightingPassShader.activate();
        lightingPassShader.setVec3("lightPosition", Mv * vec4(lights[0], 1));
        lightingPassShader.setFloat("SCR_WIDTH", windowWidth);
        lightingPassShader.setFloat("SCR_HEIGHT", windowHeight);
        lightingPassShader.setMat4("invProj", inverse(Mp));
        for (int i = 0; i < sizeof(lights) / sizeof(lights[0]); i++)
        {
            lightingPassShader.setVec3("lights[" + std::to_string(i) + "].position", Mv * vec4(lights[i * 3], 1));
        }


        glBindVertexArray(rectVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        */
        /*
        //SSR
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ssrFB);
        glClearColor(1.0, 1.0, 1.0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDepthMask(GL_FALSE);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        SSRShader.activate();
        SSRShader.setFloat("SCR_WIDTH", windowWidth);
        SSRShader.setFloat("SCR_HEIGHT", windowHeight);
        SSRShader.setMat4("invProjection", inverse(Mp));
        SSRShader.setMat4("projection", Mp);

        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glBindVertexArray(rectVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDepthMask(GL_TRUE);

        //output
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, originalFrameBuffer);
        glClearColor(0, 0, 0, 1.0f);
        glStencilMask(0xFF);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, reflectionColorBuffer);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gSpecular);

        outputShader.activate();
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glBindVertexArray(rectVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        */
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
#include <filesystem>
#include <iostream>
#include <memory>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <happly.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "path_utils.h"
#include "perspective_camera.h"
#include "renderer.h"
#include "gl_points_renderer.h"

constexpr float WINDOW_WIDTH = 1024;
constexpr float WINDOW_HEIGHT = 768;

constexpr float MOVE_SPEED = 0.001f;
constexpr float ROTATE_SPEED = 0.5f;

int main(int argc, char **argv) {
    std::cout << "Starting OpenGL pointclouds demo" << std::endl;

    // Process arguments
    std::string plyPath;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            // Show help message
            std::cout << "./opengl-pointclouds ply_file_path [-h]" << std::endl;
            std::cout << "ply_file_path: Path to .ply file used for rendering" << std::endl;
            std::cout << "-h: Show help message" << std::endl;

            return 0;
        } else if (plyPath.empty()) {
            plyPath = argv[i];
        } else {
            std::cerr << "Invalid argument: " << argv[i] << std::endl;
            return -1;
        }
    }

    if (plyPath.empty()) {
        std::cerr << "No path to .ply file given" << std::endl;
        return -1;
    }

    // Camera
    struct WindowData {
        std::shared_ptr<BaseCamera> camera = nullptr;
        bool leftMousePressed = false;
        bool rightMousePressed = false;
    };

    std::shared_ptr<BaseCamera> camera = std::make_shared<PerspectiveCamera>(glm::vec3(0.f, 0.f, 1.f), 80.f, WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 2000.f);

    WindowData windowData{};
    windowData.camera = camera;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "opengl-pointclouds", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();

        return -1;
    }

    glfwSetWindowUserPointer(window, &windowData);
    glfwMakeContextCurrent(window);
    glfwSetErrorCallback(
        [](int error, const char *description) -> void { std::cerr << "GLFW error: " << description << std::endl; });
    glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) -> void {
        if (ImGui::GetIO().WantCaptureMouse) {
            return;
        }

        WindowData *windowData = static_cast<WindowData *>(glfwGetWindowUserPointer(window));
        if (!windowData) {
            return;
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            windowData->leftMousePressed = action == GLFW_PRESS;
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            windowData->rightMousePressed = action == GLFW_PRESS;
        }
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xpos, double ypos) -> void {
        static double lastX = xpos, lastY = ypos;
        static bool firstMouse = true;

        WindowData *windowData = static_cast<WindowData *>(glfwGetWindowUserPointer(window));
        if (!windowData) {
            return;
        }

        if (firstMouse) {
            firstMouse = false;
            return;
        }

        float xRel = xpos - lastX;
        float yRel = ypos - lastY;
        lastX = xpos;
        lastY = ypos;

        if (ImGui::GetIO().WantCaptureMouse) {
            return;
        }

        if (windowData->leftMousePressed) {
            windowData->camera->translate(glm::vec3(-xRel * MOVE_SPEED, yRel * MOVE_SPEED, 0.f));
        }

        if (windowData->rightMousePressed) {
            glm::vec3 rot(0.0f);
            rot.y += xRel * ROTATE_SPEED;
            rot.x += yRel * ROTATE_SPEED;

            windowData->camera->rotate(rot);
        }
    });
    glfwSetScrollCallback(window, [](GLFWwindow *window, double xoffset, double yoffset) {
        if (ImGui::GetIO().WantCaptureMouse) {
            return;
        }

        WindowData *windowData = static_cast<WindowData *>(glfwGetWindowUserPointer(window));
        if (!windowData) {
            return;
        }

        windowData->camera->zoom(yoffset);
    });

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) -> void {
        WindowData *windowData = static_cast<WindowData *>(glfwGetWindowUserPointer(window));
        if (!windowData) {
            return;
        }

        // windowData->camera->setProjection();
        glViewport(0, 0, width, height);
    });

    // Load ply file
    std::filesystem::path path = getExecutableDirectory() / plyPath;
    happly::PLYData pointCloud{path.string()};
    std::vector<std::array<double, 3>> vPos = pointCloud.getVertexPositions();
    std::vector<glm::vec3> vertexPositions;
    for (const std::array<double, 3> &vertex : vPos) {
        vertexPositions.push_back(glm::vec3(vertex[0], vertex[1], vertex[2]));
    }

    // Create renderer
    std::unique_ptr<Renderer> renderer = std::make_unique<GlPointsRenderer>(vertexPositions, camera);
    renderer->initialize();

    // Initialize imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    int algorithm = 0;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw point cloud
        renderer->draw();

        // imgui draw
        ImGui::Begin("Settings");

        int selectedAlgorithm;
        if (ImGui::Combo("Algorithm", &selectedAlgorithm, "GL_POINTS\0Basic Compute\0High Quality Shading")) {
            if (algorithm != selectedAlgorithm) {
                // Switch algorithm
            }
        }

        ImGui::End();

        std::string path;

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    renderer->destroy();

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Finishing OpenGL pointclouds demo" << std::endl;
    return 0;
}

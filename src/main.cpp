#include <iostream>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <happly.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "perspective_camera.h"

constexpr float WINDOW_WIDTH = 1024;
constexpr float WINDOW_HEIGHT = 768;

constexpr float MOVE_SPEED = 0.001f;
constexpr float ROTATE_SPEED = 0.5f;

int main() {
    std::cout << "Starting OpenGL pointclouds demo" << std::endl;

    // Camera
    struct WindowData {
        PerspectiveCamera *camera = nullptr;
        bool leftMousePressed = false;
        bool rightMousePressed = false;
    };

    PerspectiveCamera camera =
        PerspectiveCamera{glm::vec3(0.f, 0.f, 1.f), 80.f, WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 2000.f};

    WindowData windowData{};
    windowData.camera = &camera;

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

        if (windowData->leftMousePressed) {
            windowData->camera->translate(glm::vec3(xRel * MOVE_SPEED, yRel * MOVE_SPEED, 0.f));
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
        WindowData *windowData = static_cast<WindowData *>(glfwGetWindowUserPointer(window));
        if (!windowData) {
            return;
        }

        windowData->camera->zoom(-yoffset);
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

    // Create shaders and shader programs
    auto const vsSrc = R".(
        #version 450

        layout(location = 0) in vec3 inPos;

        layout(location = 0) out vec4 outColor;

        uniform mat4 view;
        uniform mat4 proj;

        void main() {
            vec4 vertex = vec4(inPos, 1.f);       

            gl_Position = proj * view * vertex;
            outColor = vec4(0.f, 0.f, 1.f, 1.f);
        }
    ).";

    auto const fsSrc = R".(
        #version 450

        layout(location = 0) in vec4 inColor;

        layout(location = 0) out vec4 outColor;

        void main() {
            outColor = inColor;
        }
    ).";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vs, 1, &vsSrc, nullptr);
    glShaderSource(fs, 1, &fsSrc, nullptr);

    glCompileShader(vs);
    glCompileShader(fs);

    GLuint program = glCreateProgram();

    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    GLint angleLocation = glGetUniformLocation(program, "angle");
    GLint viewLocation = glGetUniformLocation(program, "view");
    GLint projLocation = glGetUniformLocation(program, "proj");

    // Load ply file
    happly::PLYData pointCloud("C:\\Users\\chsal\\Documents\\vut-fit\\pgr\\opengl-terrain\\resources\\bunny.ply");
    std::vector<std::array<double, 3>> vPos = pointCloud.getVertexPositions();

    // Create vertex buffer
    GLuint vao, vbo;

    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);

    glNamedBufferStorage(vbo, vPos.size() * sizeof(double) * 3, vPos.data(), 0);

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_DOUBLE, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(double) * 3);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.1f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw
        glUseProgram(program);

        glm::mat4 viewMatrix = windowData.camera->viewMatrix();
        glm::mat4 projectionMatrix = windowData.camera->projectionMatrix();
        glProgramUniformMatrix4fv(program, viewLocation, 1, GL_FALSE, (float *)&viewMatrix);
        glProgramUniformMatrix4fv(program, projLocation, 1, GL_FALSE, (float *)&projectionMatrix);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, vPos.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Finishing OpenGL pointclouds demo" << std::endl;
    return 0;
}

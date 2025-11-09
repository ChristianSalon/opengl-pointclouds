#include <iostream>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr float WINDOW_WIDTH = 1024;
constexpr float WINDOW_HEIGHT = 768;

constexpr float MOVE_SPEED = 0.1;

void errorCallback(int error, const char* description) {
    std::cerr << "GLFW error: " << description << std::endl;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main() {
    std::cout << "Starting OpenGL pointclouds demo" << std::endl;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "opengl-pointclouds", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();

        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetErrorCallback(errorCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Create shaders and shader programs
    auto const vsSrc = R".(
        #version 450

        layout(location = 0) out vec4 outColor;

        uniform float angle;
        uniform mat4 view;
        uniform mat4 proj;

        vec2 positions[] = {
            vec2(-0.5, -0.5), 
            vec2(0.5, -0.5), 
            vec2(-0.5, 0.5)
        };

        vec4 colors[] = {
            vec4(1.f, 0.f, 0.f, 1.f), 
            vec4(0.f, 1.f, 0.f, 1.f), 
            vec4(0.f, 0.f, 1.f, 1.f)
        };

        void main() {
            vec4 vertex = vec4(positions[gl_VertexID], 0.f, 1.f);

            float cosAngle = cos(angle);
            float sinAngle = sin(angle);

            mat4 rotY = mat4(
                cosAngle,  0.f, sinAngle, 0.f, 
                0.f, 1.f, 0.f, 0.f, 
               -sinAngle, 0.f, cosAngle, 0.f, 
                0.f, 0.f, 0.f, 1.f
            );            

            gl_Position = proj * view * rotY * vertex;
            outColor = colors[gl_VertexID];
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

    GLfloat angle = 0.f;
    glm::mat4 projMatrix = glm::perspective(glm::radians(60.f), WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.f);
    glm::mat4 viewMatrix = glm::lookAt(glm::vec3(0.f, 0.f, 5.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));

    GLint angleLocation = glGetUniformLocation(program, "angle");
    GLint viewLocation = glGetUniformLocation(program, "view");
    GLint projLocation = glGetUniformLocation(program, "proj");

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.1f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw
        glUseProgram(program);

        glUniform1f(angleLocation, angle);
        glProgramUniformMatrix4fv(program, viewLocation, 1, GL_FALSE, (float*) &viewMatrix);
        glProgramUniformMatrix4fv(program, projLocation, 1, GL_FALSE, (float*) &projMatrix);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        angle += 1.f / 10'000.f;

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

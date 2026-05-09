#include <filesystem>
#include <iostream>
#include <memory>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <happly.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imfilebrowser.h"

#include "path_utils.h"
#include "perspective_camera.h"
#include "renderer.h"
#include "gl_points_renderer.h"
#include "basic_compute_renderer.h"
#include "high_quality_renderer.h"
#include "triangle_mesh_renderer.h"
#include "poisson_reconstruction_renderer.h"
#include "octree_builder.h"

constexpr float WINDOW_WIDTH = 1024;
constexpr float WINDOW_HEIGHT = 768;
constexpr float MOVE_SPEED = 0.005f;
constexpr float ROTATE_SPEED = 0.4f;

enum RenderAlgorithm { SIMPLE_POINTS, BASIC_COMPUTE, HIGH_QUALITY, TRIANGLE_MESH, POISSON };
int renderAlgorithm = SIMPLE_POINTS;

struct WindowData {
    std::shared_ptr<PerspectiveCamera> camera = nullptr;
    bool leftMousePressed = false;
    bool rightMousePressed = false;
    int width = WINDOW_WIDTH;
    int height = WINDOW_HEIGHT;
};

std::shared_ptr<PerspectiveCamera> camera = nullptr;
WindowData windowData{};

std::unique_ptr<Renderer> renderer = nullptr;
std::shared_ptr<Renderer::Params> rendererParams = std::make_shared<Renderer::Params>();
std::shared_ptr<PoissonReconstructionRenderer::PoissonParams> poissonRendererParams = std::make_shared<PoissonReconstructionRenderer::PoissonParams>();
size_t pointsRendered;

std::shared_ptr<OctreeBuilder::OctreeNode> octree = nullptr;
bool hasColor = false;
bool isPointCloudSelected = false;
std::string pointCloudPath = "";

void loadPointCloud(std::string path) {
    if (renderAlgorithm == TRIANGLE_MESH) {
        // Let CGAL handle loading
        if (TriangleMeshRenderer *meshRenderer = dynamic_cast<TriangleMeshRenderer*>(renderer.get())) {
            meshRenderer->setPointCloud(path);
            hasColor = meshRenderer->hasColor();
        }
    } else if (renderAlgorithm == POISSON) {
        // Let CGAL handle loading
        if (PoissonReconstructionRenderer *poissonRenderer = dynamic_cast<PoissonReconstructionRenderer*>(renderer.get())) {
            poissonRenderer->setPointCloud(path);
            hasColor = poissonRenderer->hasColor();
        }
    } else {
        // Load using happly
        happly::PLYData pointCloud{path};
        std::vector<glm::vec4> vertexPositions;
        std::vector<glm::u8vec4> vertexColors;

        // Load positions
        std::vector<std::array<double, 3>> vPos = pointCloud.getVertexPositions();
        for (const std::array<double, 3> &vertex : vPos) {
            vertexPositions.push_back(glm::vec4(vertex[0], vertex[1], vertex[2], 1.0f));
        }

        // Try to load color
        try {
            std::vector<std::array<unsigned char, 3>> vCol = pointCloud.getVertexColors();
            for (const std::array<unsigned char, 3> &color : vCol) {
                vertexColors.push_back(glm::u8vec4(color[0], color[1], color[2], 1.0f));
            }

            hasColor = true;
        } catch (const std::exception &e) {
            std::cout << "Point cloud " << path << " does not support vertex colors" << std::endl;
            hasColor = false;
        }

        OctreeBuilder octreeBuilder;
        octree = octreeBuilder.build(std::move(vertexPositions), std::move(vertexColors));
        renderer->setPointCloud(octree);
    }

    pointCloudPath = path;
    isPointCloudSelected = true;
}

void exportMesh(std::string path) {
    if (renderAlgorithm == POISSON) {
        if (PoissonReconstructionRenderer *poissonRenderer = dynamic_cast<PoissonReconstructionRenderer*>(renderer.get())) {
            poissonRenderer->exportMesh(path);
        }
    } else {
        throw std::runtime_error("Current renderer does not support exporting triangle mesh");
    }
}

void updateRenderer() {
    static int lastAlgorithm = SIMPLE_POINTS;

    if (renderAlgorithm == lastAlgorithm) return;

    glFinish();

    renderer->destroy();
    renderer.reset();

    if (renderAlgorithm == SIMPLE_POINTS) {
        renderer = std::make_unique<GlPointsRenderer>(camera, rendererParams);
        renderer->setPointCloud(octree);
    } else if (renderAlgorithm == BASIC_COMPUTE) {
        renderer = std::make_unique<BasicComputeRenderer>(camera, rendererParams);
        renderer->setPointCloud(octree);
    } else if (renderAlgorithm == HIGH_QUALITY) {
        renderer = std::make_unique<HighQualityRenderer>(camera, rendererParams);
        renderer->setPointCloud(octree);
    } else if (renderAlgorithm == TRIANGLE_MESH) {
        renderer = std::make_unique<TriangleMeshRenderer>(camera, rendererParams);
        if (TriangleMeshRenderer *meshRenderer = dynamic_cast<TriangleMeshRenderer*>(renderer.get())) {
            meshRenderer->initialize();
            meshRenderer->setPointCloud(pointCloudPath);
        }
    } else if (renderAlgorithm == POISSON) {
        renderer = std::make_unique<PoissonReconstructionRenderer>(camera, rendererParams, poissonRendererParams);
        if (PoissonReconstructionRenderer *poissonRenderer = dynamic_cast<PoissonReconstructionRenderer*>(renderer.get())) {
            poissonRenderer->initialize();
            poissonRenderer->setPointCloud(pointCloudPath);
        }
    }

    renderer->setWindowDimensions(windowData.width, windowData.height);

    lastAlgorithm = renderAlgorithm;
}

void RenderUI(ImGui::FileBrowser &fileBrowser, ImGui::FileBrowser &fileSaveBrowser) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::Begin("Side Panel");

    if (ImGui::CollapsingHeader("1. Global Setting", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Select .ply file", ImVec2(-1, 0))) {
            fileBrowser.Open();
        }

        if (isPointCloudSelected) {
            ImGui::Text("Algorithm");
            const char *algorithms[] = {"GL Points", "Basic Compute", "High Quality", "Triangle Mesh", "Poisson"};
            if (ImGui::Combo("##algorithm", &renderAlgorithm, algorithms, IM_ARRAYSIZE(algorithms))) {
                updateRenderer();
            }
        
            ImGui::Text("Point Color");
            ImGui::ColorEdit3("##pointColor", glm::value_ptr(rendererParams->pointColor));

            if (hasColor) {
                ImGui::Checkbox("Use Default Color", &(rendererParams->useDefaultColor));
            } else {
                bool checked = true;

                ImGui::BeginDisabled();
                ImGui::Checkbox("Use Default Color", &checked);
                ImGui::EndDisabled();
            }
        }
    }

    if (isPointCloudSelected) {
        if (ImGui::CollapsingHeader("2. Point Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Max Pixel Error");
            ImGui::SliderFloat("##maxPixelError", &(rendererParams->maxPixelError), 0.0f, 20.0f);

            ImGui::Text("Point Size");
            ImGui::SliderFloat("##pointSize", &(rendererParams->pointSize), 0.1f, 10.0f);

            ImGui::Text("Hole Filling Iterations");
            ImGui::SliderInt("##holeFillingIterations", &(rendererParams->holeFillingIterations), 1, 20);

            ImGui::Text("Hole Filling Influence");
            ImGui::SliderFloat("##holeFillingInfluence", &(rendererParams->holeFillingInfluence), 0.0001f, 1.0f);

            ImGui::Checkbox("Enable Hole Filling", &(rendererParams->enableHoleFilling));

            ImGui::Text("EDL Levels");
            ImGui::SliderInt("##edlLevels", &(rendererParams->edlLevels), 1, 15);

            ImGui::Text("EDL Shading Factor");
            ImGui::SliderFloat("##edlShadingFactor", &(rendererParams->shadingFactor), 0.0f, 5.0f);

            ImGui::Text("EDL Shading Strength");
            ImGui::SliderFloat("##edlShadingStrength", &(rendererParams->shadingStrength), 0.0f, 2.0f);

            ImGui::Checkbox("Enable EDL", &(rendererParams->enableEdl));
        }

        if (ImGui::CollapsingHeader("3. Poisson Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Remove Outliers");
            ImGui::Checkbox("##removeOutliers", &poissonRendererParams->removeOutliers);
            ImGui::Text("Outlier Threshold %");
            ImGui::SliderFloat("##outlierThreshold", &poissonRendererParams->outlierThreshold, 1.0f, 20.0f);
            ImGui::Text("Outlier Neighbors");
            ImGui::SliderInt("##outlierNeighbors", &poissonRendererParams->outlierNeighbors, 1, 50);
            
            ImGui::Text("Simplify");
            ImGui::Checkbox("##simplify", &poissonRendererParams->simplify);
            ImGui::Text("Simplification Ratio");
            ImGui::SliderFloat("##simplificationRatio", &poissonRendererParams->simplifyRatio, 0.5f, 5.0f);
            ImGui::Text("Simplification Heighbors");
            ImGui::SliderInt("##simplificationNeighbors", &poissonRendererParams->simplifyNeighbors, 1, 50);
            
            ImGui::Text("Smoothing");
            ImGui::Checkbox("##smoothing", &poissonRendererParams->smooth);
            ImGui::Text("Smoothing Neighbors");
            ImGui::SliderInt("##smoothingNeighbors", &poissonRendererParams->smoothNeighbors, 1, 50);
            
            ImGui::Text("Fill Holes");
            ImGui::Checkbox("##fillHoles", &poissonRendererParams->fillHoles);
            ImGui::Text("Max Edges");
            ImGui::SliderInt("##maxEdges", &poissonRendererParams->maxHoleEdges, 3, 500);

            if (ImGui::Button("Reconstruct Surface", ImVec2(-1, 0))) {
                if (renderAlgorithm == POISSON) {
                    if (PoissonReconstructionRenderer *poissonRenderer = dynamic_cast<PoissonReconstructionRenderer*>(renderer.get())) {
                        poissonRenderer->reconstruct();
                    }
                }
            }

            if (ImGui::Button("Export Mesh to .ply", ImVec2(-1, 0))) {
                fileSaveBrowser.Open();
            }
        }

        if (ImGui::CollapsingHeader("4. Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Elements Rendered: %d", pointsRendered);
        }
    }

    ImGui::End();
}

int main(int argc, char **argv) {
    std::cout << "Starting OpenGL pointclouds demo" << std::endl;

    // Process arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            // Show help message
            std::cout << "./opengl-pointclouds [-h]" << std::endl;
            std::cout << "-h: Show help message" << std::endl;

            return 0;
        } else {
            std::cerr << "Invalid argument: " << argv[i] << std::endl;
            return -1;
        }
    }

    // Camera
    camera = std::make_shared<PerspectiveCamera>(glm::vec3(0.f, 0.f, 1.f), 80.f, WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 2000.f);
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

        windowData->width = width;
        windowData->height = height;

        glViewport(0, 0, width, height);
    });

    // Initialize imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    ImGui::FileBrowser fileBrowser;
    fileBrowser.SetTitle("Select Point Cloud");
    fileBrowser.SetTypeFilters({".ply"});
    
    ImGui::FileBrowser fileSaveBrowser(ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir);
    fileSaveBrowser.SetTitle("Save Reconstructed Mesh");
    fileSaveBrowser.SetTypeFilters({".ply"});

    // Create renderer
    renderer = std::make_unique<GlPointsRenderer>(camera, rendererParams);
    renderer->setWindowDimensions(windowData.width, windowData.height);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(ImVec2(300.0f, viewport->Size.y));

        RenderUI(fileBrowser, fileSaveBrowser);
        fileBrowser.Display();
        if (fileBrowser.HasSelected()) {
            std::cout << "Selected point cloud: " << fileBrowser.GetSelected().string() << std::endl;
            loadPointCloud(fileBrowser.GetSelected().string());

            fileBrowser.ClearSelected();
        }
        fileSaveBrowser.Display();
        if (fileSaveBrowser.HasSelected()) {
            std::cout << "Exported mesh: " << fileSaveBrowser.GetSelected().string() << std::endl;
            exportMesh(fileSaveBrowser.GetSelected().string());

            fileSaveBrowser.ClearSelected();
        }

        // Draw point cloud
        if (isPointCloudSelected) {
            renderer->setWindowDimensions(windowData.width, windowData.height);
            pointsRendered = renderer->draw();
        }

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

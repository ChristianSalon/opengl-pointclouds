cmake_minimum_required(VERSION 3.10)
project(opengl-pointclouds)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(INCLUDES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SHADERS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(IMGUI_PATH ${LIBS_PATH}/imgui)

# executable source files
set(SOURCES
    ${SRC_PATH}/main.cpp
    ${SRC_PATH}/path_utils.cpp
    ${SRC_PATH}/base_camera.cpp
    ${SRC_PATH}/perspective_camera.cpp
    ${SRC_PATH}/renderer.cpp
    ${SRC_PATH}/gl_points_renderer.cpp
    ${SRC_PATH}/basic_compute_renderer.cpp
    ${SRC_PATH}/high_quality_renderer.cpp
    ${SRC_PATH}/triangle_mesh_renderer.cpp
    ${SRC_PATH}/poisson_reconstruction_renderer.cpp
    ${SRC_PATH}/octree_builder.cpp
    ${SRC_PATH}/rolling_ball_renderer.cpp
)

# executable header files
set(HEADERS
    ${INCLUDES_PATH}/path_utils.h
    ${INCLUDES_PATH}/base_camera.h
    ${INCLUDES_PATH}/perspective_camera.h
    ${INCLUDES_PATH}/renderer.h
    ${INCLUDES_PATH}/gl_points_renderer.h
    ${INCLUDES_PATH}/basic_compute_renderer.h
    ${INCLUDES_PATH}/high_quality_renderer.h
    ${INCLUDES_PATH}/triangle_mesh_renderer.h
    ${INCLUDES_PATH}/poisson_reconstruction_renderer.h
    ${INCLUDES_PATH}/octree_builder.h
    ${INCLUDES_PATH}/rolling_ball_renderer.h
)

# executable glsl shaders
set(SHADERS
    ${SHADERS_PATH}/gl_points.vert
    ${SHADERS_PATH}/gl_points.frag
    ${SHADERS_PATH}/basic_compute_render.comp
    ${SHADERS_PATH}/basic_compute_hole_filling.comp
    ${SHADERS_PATH}/basic_compute_edl.comp
    ${SHADERS_PATH}/basic_compute_resolve.comp
    ${SHADERS_PATH}/basic_compute_quad.vert
    ${SHADERS_PATH}/basic_compute_quad.frag
    ${SHADERS_PATH}/high_quality_depth.comp
    ${SHADERS_PATH}/high_quality_color.comp
    ${SHADERS_PATH}/high_quality_resolve.comp
    ${SHADERS_PATH}/high_quality_hole_filling.comp
    ${SHADERS_PATH}/high_quality_edl.comp
    ${SHADERS_PATH}/high_quality_second_resolve.comp
    ${SHADERS_PATH}/high_quality_quad.vert
    ${SHADERS_PATH}/high_quality_quad.frag
    ${SHADERS_PATH}/triangle_mesh.vert
    ${SHADERS_PATH}/triangle_mesh.frag
)

# imgui source files
set(IMGUI_SOURCES
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_demo.cpp
    ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp
    ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
)

# executable
set(EXECUTABLE_NAME ${PROJECT_NAME})
add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS} ${SHADERS})

# glad
add_library(glad ${LIBS_PATH}/glad/src/glad.c)
target_include_directories(glad PUBLIC ${LIBS_PATH}/glad/include)
target_include_directories(${EXECUTABLE_NAME} PUBLIC ${LIBS_PATH}/glad/include)

# glfw
add_subdirectory(${LIBS_PATH}/glfw)

# imgui
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${IMGUI_PATH}
    ${IMGUI_PATH}/backends
    ${LIBS_PATH}/glfw/include
)
target_link_libraries(imgui PUBLIC glfw glad)

# imgui-filebrowser
target_include_directories(${EXECUTABLE_NAME} PUBLIC ${LIBS_PATH}/imgui-filebrowser)

# glm
add_subdirectory(${LIBS_PATH}/glm)

# happly - parsing PLY files
target_include_directories(${EXECUTABLE_NAME} PUBLIC ${LIBS_PATH}/happly)

# cgal
find_package(CGAL REQUIRED COMPONENTS Core)
find_package(Eigen3 REQUIRED)

# executable dependencies
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${INCLUDES_PATH})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE glad glfw imgui glm::glm CGAL::CGAL Eigen3::Eigen opengl32 gdi32 user32 shell32 kernel32)

# copy resources to executable directory
message(STATUS "Copying resources to build directory")
add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/resources"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/resources/bunny.ply" "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/resources"
)

# copy shaders to executable directory
foreach(SHADER_FILE ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)

    message(STATUS "Copying shader ${SHADER_NAME} to build directory")
    add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/shaders"
        COMMAND ${CMAKE_COMMAND} -E copy "${SHADER_FILE}" "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/shaders"
    )
endforeach()

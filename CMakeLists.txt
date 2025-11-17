cmake_minimum_required(VERSION 3.10)
project(opengl-pointclouds)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(INCLUDES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)

# executable source files
set(SOURCES
    ${SRC_PATH}/main.cpp
    ${SRC_PATH}/base_camera.cpp
    ${SRC_PATH}/perspective_camera.cpp
)

# executable header files
set(HEADERS
    ${INCLUDES_PATH}/base_camera.h
    ${INCLUDES_PATH}/perspective_camera.h
)

# executable
set(EXECUTABLE_NAME ${PROJECT_NAME})
add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS})

# glad
add_library(glad ${LIBS_PATH}/glad/src/glad.c)
target_include_directories(glad PRIVATE ${LIBS_PATH}/glad/include)
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${LIBS_PATH}/glad/include)

# glfw
add_subdirectory(${LIBS_PATH}/glfw)

# glm
add_subdirectory(${LIBS_PATH}/glm)

# happly - parsing PLY files
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${LIBS_PATH}/happly)

# executable dependencies
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${INCLUDES_PATH})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE glad glfw glm::glm opengl32 gdi32 user32 shell32 kernel32)

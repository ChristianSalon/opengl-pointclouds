cmake_minimum_required(VERSION 3.10)
project(opengl-pointclouds)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(INCLUDES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)

# executable include files
# set(INCLUDES
#     ${INCLUDES_PATH}/
# )

# executable source files
set(SOURCES
    ${SRC_PATH}/main.cpp
)

# executable
set(EXECUTABLE_NAME ${PROJECT_NAME})
add_executable(${EXECUTABLE_NAME} ${SOURCES})

# glad
add_library(glad ${LIBS_PATH}/glad/src/glad.c)
target_include_directories(glad PRIVATE ${LIBS_PATH}/glad/include)
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${LIBS_PATH}/glad/include)

# glfw
add_subdirectory(${LIBS_PATH}/glfw)

# glm
add_subdirectory(${LIBS_PATH}/glm)

# executable dependencies
target_link_libraries(${EXECUTABLE_NAME} PRIVATE glad glfw glm::glm opengl32 gdi32 user32 shell32 kernel32)

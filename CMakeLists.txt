cmake_minimum_required(VERSION 3.10)
project(opengl-terrain)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(INCLUDES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)

# terrain demo executable include files
# set(INCLUDES
#     ${INCLUDES_PATH}/
# )

# terrain demo executable source files
set(SOURCES
    ${SRC_PATH}/main.cpp
)

# terrain demo executable
set(EXECUTABLE_NAME opengl-terrain)
add_executable(${EXECUTABLE_NAME} ${SOURCES})

# glad
add_library(glad ${LIBS_PATH}/glad/src/glad.c)
target_include_directories(glad PRIVATE ${LIBS_PATH}/glad/include)
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${LIBS_PATH}/glad/include)

# glfw
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${LIBS_PATH}/glfw/include)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBS_PATH}/glfw/glfw3.lib)

# terrain demo dependencies
target_link_libraries(${EXECUTABLE_NAME} PRIVATE glad opengl32 gdi32 user32 shell32 kernel32)

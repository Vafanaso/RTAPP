cmake_minimum_required(VERSION 3.10)
project(RaytracerGUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------
# GLAD
# -------------------------
add_library(glad src/glad.c)
target_include_directories(glad PUBLIC include)

# -------------------------
# ImGui
# -------------------------
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# -------------------------
# Raytracer sources
# -------------------------
file(GLOB RAYTRACER_SRC
    raytracer/*.cpp
)

# -------------------------
# Main App
# -------------------------
add_executable(gui_app
    src/main.cpp
    ${RAYTRACER_SRC}
)

target_include_directories(gui_app PUBLIC
    include
    raytracer
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/glm

)

target_link_libraries(gui_app
    glad
    imgui
    glfw
    GL
    dl
    pthread
)

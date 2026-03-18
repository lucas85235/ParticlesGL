cmake_minimum_required(VERSION 3.20)
project(ParticleGL VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# We need to build glad with C
set(CMAKE_C_STANDARD 99)

include(cmake/dependencies.cmake)

# Executable
add_executable(ParticleGL
    src/main.cpp
    src/core/Application.cpp
    src/core/AssetManager.cpp
    src/core/Window.cpp
    src/ecs/Registry.cpp
    src/renderer/Shader.cpp
    src/renderer/Mesh.cpp
    src/renderer/InstanceBuffer.cpp
    src/renderer/Framebuffer.cpp
    src/renderer/Camera.cpp
    src/renderer/Renderer.cpp
    src/particles/ParticlePool.cpp
    src/ecs/systems/ParticleSystem.cpp
    src/serialization/SceneSerializer.cpp
    src/ui/UILayer.cpp
    src/ui/ScenePanel.cpp
    src/ui/InspectorPanel.cpp
    src/ui/StatsPanel.cpp
    src/ui/AssetsPanel.cpp
    src/ui/ViewportPanel.cpp
    src/ui/MaterialsPanel.cpp
    src/ui/ViewportPanel.cpp
    src/ui/MaterialsPanel.cpp
)

target_include_directories(ParticleGL PRIVATE src)

if(MSVC)
    target_compile_options(ParticleGL PRIVATE /W3 /WX)
else()
    target_compile_options(ParticleGL PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)
endif()

# Link dependencies for Main Engine
target_link_libraries(ParticleGL PRIVATE
    glad
    glfw
    glm::glm
    imgui
    nlohmann_json::nlohmann_json
    tinyobjloader::tinyobjloader
)

# -----------------------------------------------------------------------------
# Standalone Compute Shader Demo Executable
# -----------------------------------------------------------------------------
add_executable(ParticleGL_ComputeDemo
    src/demo/main_compute.cpp
    src/demo/ComputeApp.cpp
    src/demo/ComputeExample.cpp
    src/core/Window.cpp
    src/renderer/ComputeShader.cpp
    src/renderer/ShaderStorageBuffer.cpp
)

target_include_directories(ParticleGL_ComputeDemo PRIVATE src)

if(MSVC)
    target_compile_options(ParticleGL_ComputeDemo PRIVATE /W3 /WX)
else()
    target_compile_options(ParticleGL_ComputeDemo PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)
endif()

target_link_libraries(ParticleGL_ComputeDemo PRIVATE
    glad
    glfw
    glm::glm
    imgui
)

# Testing
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

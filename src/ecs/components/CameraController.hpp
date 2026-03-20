#pragma once
#include "renderer/Camera.hpp"

namespace ParticleGL::ECS::Components {

struct CameraController {
    Renderer::Camera* targetCamera = nullptr;
    float moveSpeed = 5.0f;
    float lookSpeed = 0.2f;
    
    // Internal state
    bool isViewportHovered = false;
    bool rightMouseDown = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;
};

} // namespace ParticleGL::ECS::Components

#include "CameraControllerSystem.hpp"
#include "../components/CameraController.hpp"
#include "../components/Transform.hpp"
#include <imgui.h>
#include "../../core/Logger.hpp"

namespace ParticleGL::ECS::Systems {

void CameraControllerSystem::update(Registry &registry, float dt) {
  auto view = registry.getEntitiesWith<ECS::Components::CameraController>();
  static int frameCount = 0;
  frameCount++;

  if (view.empty() && frameCount % 120 == 0) {
      PGL_WARN("CameraSystem: No entities with CameraController found!");
  }

  for (auto entity : view) {
    auto &controller = registry.getComponent<ECS::Components::CameraController>(entity);
    if (!controller.targetCamera) continue;

    ImGuiIO &io = ImGui::GetIO();
    bool isRightMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);

    if (frameCount % 60 == 0) {
        PGL_INFO("CameraSystem tick. isRightMouseDown=" << isRightMouseDown << " WantCaptureMouse=" << io.WantCaptureMouse);
    }

    auto &cam = *controller.targetCamera;

    // Movement is independent of right mouse click so user can test W/A/S/D
    glm::vec3 pos = cam.getPosition();
    glm::vec3 front = cam.getFront();
    glm::vec3 right = cam.getRight();
    glm::vec3 up = cam.getUp();
    float velocity = controller.moveSpeed * dt;

    if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) velocity *= 2.5f;

    bool moved = false;
    if (ImGui::IsKeyDown(ImGuiKey_W)) { pos += front * velocity; moved = true; }
    if (ImGui::IsKeyDown(ImGuiKey_S)) { pos -= front * velocity; moved = true; }
    if (ImGui::IsKeyDown(ImGuiKey_A)) { pos -= right * velocity; moved = true; }
    if (ImGui::IsKeyDown(ImGuiKey_D)) { pos += right * velocity; moved = true; }
    if (ImGui::IsKeyDown(ImGuiKey_E)) { pos += up * velocity; moved = true; }
    if (ImGui::IsKeyDown(ImGuiKey_Q)) { pos -= up * velocity; moved = true; }

    if (moved) {
        PGL_INFO("Camera Moved. New Pos: " << pos.x << ", " << pos.y << ", " << pos.z);
    }

    if (isRightMouseDown) {
      if (!controller.rightMouseDown) {
        // Just clicked this frame
        controller.lastMouseX = io.MousePos.x;
        controller.lastMouseY = io.MousePos.y;
        controller.rightMouseDown = true;
      }

      float deltaX = io.MousePos.x - controller.lastMouseX;
      float deltaY = io.MousePos.y - controller.lastMouseY;
      controller.lastMouseX = io.MousePos.x;
      controller.lastMouseY = io.MousePos.y;

      // Apply rotation
      float pitch = cam.getPitch() - deltaY * controller.lookSpeed;
      float yaw = cam.getYaw() + deltaX * controller.lookSpeed;
      cam.setRotation(pitch, yaw);
      
      if (deltaX != 0.0f || deltaY != 0.0f) {
           PGL_INFO("Camera Rotated. Pitch=" << pitch << " Yaw=" << yaw);
      }
    } else {
      controller.rightMouseDown = false;
    }

    cam.setPosition(pos);

    // Sync Transform if exists
    if (registry.hasComponent<ECS::Components::Transform>(entity)) {
      auto &transform = registry.getComponent<ECS::Components::Transform>(entity);
      transform.position = pos;
    }
  }
}

} // namespace ParticleGL::ECS::Systems

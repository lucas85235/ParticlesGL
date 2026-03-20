#include "InspectorPanel.hpp"
#include <imgui.h>

#include "../ecs/components/Lifetime.hpp"
#include "../ecs/components/ParticleEmitter.hpp"
#include "../ecs/components/Renderable.hpp"
#include "../ecs/components/Transform.hpp"
#include "../ecs/components/CameraController.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace ParticleGL::UI {

namespace {

void DrawTransformInspector(ECS::Registry &reg, ECS::Entity entity) {
  auto &transform = reg.getComponent<ECS::Components::Transform>(entity);
  ImGui::DragFloat3("Position", glm::value_ptr(transform.position), 0.1f);

  // Euler angles for rotation
  glm::vec3 euler = glm::degrees(glm::eulerAngles(transform.rotation));
  if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 1.0f)) {
    transform.rotation = glm::quat(glm::radians(euler));
  }

  ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.1f);
}

void DrawParticleEmitterInspector(ECS::Registry &reg, ECS::Entity entity) {
  using ECS::Components::ParticleBlendMode;
  auto &emitter = reg.getComponent<ECS::Components::ParticleEmitter>(entity);

  if (ImGui::TreeNodeEx("Emission & Lifetime", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::DragFloat("Emission Rate", &emitter.emissionRate, 1.0f, 0.0f, 10000.0f);
    ImGui::DragFloat("Life Time", &emitter.particleLifetime, 0.1f, 0.0f, 100.0f);
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Motion", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::DragFloat3("Initial Velocity", glm::value_ptr(emitter.initialVelocity), 0.1f);
    ImGui::DragFloat("Spread Angle", &emitter.spreadAngle, 1.0f, 0.0f, 180.0f);
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Appearance", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::ColorEdit4("Start Color", glm::value_ptr(emitter.startColor));
    ImGui::ColorEdit4("End Color", glm::value_ptr(emitter.endColor));
    
    const char *blendItems[] = {"Additive (fire/plasma)", "Alpha (smoke/cloud)"};
    int blendIdx = static_cast<int>(emitter.blendMode);
    if (ImGui::Combo("Blend Mode", &blendIdx, blendItems, 2)) {
      emitter.blendMode = static_cast<ParticleBlendMode>(blendIdx);
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Physics & Advanced", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::DragFloat("Turbulence", &emitter.turbulence, 0.1f, 0.0f, 50.0f);
    ImGui::Checkbox("Enable Collision", &emitter.collisionEnabled);
    if (emitter.collisionEnabled) {
      ImGui::Indent();
      ImGui::DragFloat("Bounciness",   &emitter.bounciness,  0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Friction",     &emitter.friction,    0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Floor Height", &emitter.floorHeight, 0.1f, -100.0f, 100.0f);
      ImGui::Unindent();
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Sub-Emitters", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Enable Sub-Emitter", &emitter.subEmitterEnabled);
    if (emitter.subEmitterEnabled) {
      ImGui::Indent();
      
      int childId = (emitter.childEmitterEntity == std::numeric_limits<uint32_t>::max()) ? -1 : static_cast<int>(emitter.childEmitterEntity);
      if (ImGui::InputInt("Child Entity ID (-1 to clear)", &childId)) {
        if (childId < 0) {
          emitter.childEmitterEntity = std::numeric_limits<uint32_t>::max();
        } else {
          emitter.childEmitterEntity = static_cast<uint32_t>(childId);
        }
      }
      
      int spawnCount = static_cast<int>(emitter.spawnCountOnDeath);
      if (ImGui::DragInt("Spawn On Death", &spawnCount, 1.0f, 1, 100)) {
        emitter.spawnCountOnDeath = static_cast<uint32_t>(spawnCount);
      }
      
      ImGui::DragFloat("Velocity Scale", &emitter.childSpeedScale, 0.05f, 0.0f, 5.0f);
      
      ImGui::Unindent();
    }
    ImGui::TreePop();
  }
}

void DrawLifetimeInspector(ECS::Registry &reg, ECS::Entity entity) {
  auto &lifetime = reg.getComponent<ECS::Components::Lifetime>(entity);
  ImGui::DragFloat("Max Lifetime", &lifetime.max, 0.1f, 0.0f, 1000.0f);
  ImGui::Text("Active: %s", lifetime.active ? "True" : "False");
}

void DrawRenderableInspector(ECS::Registry &reg, ECS::Entity entity) {
  auto &renderable = reg.getComponent<ECS::Components::Renderable>(entity);

  char materialIdBuf[256];
  strncpy(materialIdBuf, renderable.materialId.c_str(), sizeof(materialIdBuf));
  if (ImGui::InputText("Material ID", materialIdBuf, sizeof(materialIdBuf))) {
    renderable.materialId = materialIdBuf;
  }
}

void DrawCameraControllerInspector(ECS::Registry &reg, ECS::Entity entity) {
  auto &camCtrl = reg.getComponent<ECS::Components::CameraController>(entity);
  ImGui::DragFloat("Move Speed", &camCtrl.moveSpeed, 0.1f, 0.0f, 100.0f);
  ImGui::DragFloat("Look Speed", &camCtrl.lookSpeed, 0.01f, 0.0f, 5.0f);
  ImGui::TextDisabled("Hold Right-Mouse in Viewport to fly (WASD/Q/E)");
}

template <typename T>
void DrawComponent(const char *name, ECS::Registry &registry,
                   ECS::Entity entity,
                   void (*drawFunc)(ECS::Registry &, ECS::Entity),
                   bool canRemove = true) {
  if (registry.hasComponent<T>(entity)) {
    bool removeComponent = false;
    if (ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen)) {
      if (canRemove && ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) {
          removeComponent = true;
        }
        ImGui::EndPopup();
      }
      drawFunc(registry, entity);
    } else {
      if (canRemove && ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) {
          removeComponent = true;
        }
        ImGui::EndPopup();
      }
    }
    if (removeComponent) {
      registry.removeComponent<T>(entity);
    }
  }
}

} // anonymous namespace

void InspectorPanel::onImGuiRender() {
  ImGui::Begin("Inspector");

  if (registry_ && selected_entity_) {
    ECS::Entity entity = *selected_entity_;

    if (ImGui::Button("Add Component")) {
      ImGui::OpenPopup("AddComponent");
    }

    if (ImGui::BeginPopup("AddComponent")) {
      if (!registry_->hasComponent<ECS::Components::Transform>(entity)) {
        if (ImGui::MenuItem("Transform")) {
          registry_->addComponent<ECS::Components::Transform>(
              entity, ECS::Components::Transform{});
          ImGui::CloseCurrentPopup();
        }
      }

      if (!registry_->hasComponent<ECS::Components::ParticleEmitter>(entity)) {
        if (ImGui::MenuItem("Particle Emitter")) {
          registry_->addComponent<ECS::Components::ParticleEmitter>(
              entity, ECS::Components::ParticleEmitter{});
          ImGui::CloseCurrentPopup();
        }
      }

      if (!registry_->hasComponent<ECS::Components::Lifetime>(entity)) {
        if (ImGui::MenuItem("Lifetime")) {
          registry_->addComponent<ECS::Components::Lifetime>(
              entity, ECS::Components::Lifetime{});
          ImGui::CloseCurrentPopup();
        }
      }

      if (!registry_->hasComponent<ECS::Components::Renderable>(entity)) {
        if (ImGui::MenuItem("Renderable")) {
          registry_->addComponent<ECS::Components::Renderable>(
              entity, ECS::Components::Renderable{});
          ImGui::CloseCurrentPopup();
        }
      }

      if (!registry_->hasComponent<ECS::Components::CameraController>(entity)) {
        if (ImGui::MenuItem("Camera Controller")) {
          registry_->addComponent<ECS::Components::CameraController>(
              entity, ECS::Components::CameraController{});
          ImGui::CloseCurrentPopup();
        }
      }

      ImGui::EndPopup();
    }

    ImGui::Separator();

    DrawComponent<ECS::Components::Transform>("Transform", *registry_, entity,
                                              DrawTransformInspector,
                                              false); // cannot remove Transform
    DrawComponent<ECS::Components::Renderable>("Renderable", *registry_, entity,
                                               DrawRenderableInspector, true);
    DrawComponent<ECS::Components::ParticleEmitter>(
        "Particle Emitter", *registry_, entity, DrawParticleEmitterInspector,
        true);
    DrawComponent<ECS::Components::Lifetime>("Lifetime", *registry_, entity,
                                             DrawLifetimeInspector, true);
    DrawComponent<ECS::Components::CameraController>(
        "Camera Controller", *registry_, entity, DrawCameraControllerInspector,
        true);
  } else {
    ImGui::Text("No entity selected");
  }

  ImGui::End();
}

} // namespace ParticleGL::UI

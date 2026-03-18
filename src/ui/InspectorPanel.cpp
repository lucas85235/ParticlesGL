#include "InspectorPanel.hpp"
#include <imgui.h>

#include "../ecs/components/Lifetime.hpp"
#include "../ecs/components/ParticleEmitter.hpp"
#include "../ecs/components/Renderable.hpp"
#include "../ecs/components/Transform.hpp"

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
  auto &emitter = reg.getComponent<ECS::Components::ParticleEmitter>(entity);
  ImGui::DragFloat("Emission Rate", &emitter.emissionRate, 1.0f, 0.0f,
                   10000.0f);
  ImGui::DragFloat3("Initial Velocity", glm::value_ptr(emitter.initialVelocity),
                    0.1f);
  ImGui::DragFloat("Spread Angle", &emitter.spreadAngle, 1.0f, 0.0f, 180.0f);

  ImGui::ColorEdit4("Start Color", glm::value_ptr(emitter.startColor));
  ImGui::ColorEdit4("End Color", glm::value_ptr(emitter.endColor));

  ImGui::DragFloat("Life Time", &emitter.particleLifetime, 0.1f, 0.0f, 100.0f);
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
  } else {
    ImGui::Text("No entity selected");
  }

  ImGui::End();
}

} // namespace ParticleGL::UI

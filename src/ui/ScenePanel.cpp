#include "ScenePanel.hpp"
#include "../ecs/components/Transform.hpp"
#include "../serialization/SceneSerializer.hpp"
#include <cstdio>
#include <imgui.h>

namespace ParticleGL::UI {

void ScenePanel::onImGuiRender() {
  ImGui::Begin("Scene Hierarchy");

  if (registry_) {
    if (ImGui::Button("Save Scene")) {
      Serialization::SceneSerializer serializer(registry_);
      serializer.serialize("scene.json");
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Scene")) {
      Serialization::SceneSerializer serializer(registry_);
      if (serializer.deserialize("scene.json")) {
        selected_entity_ = std::nullopt;
      }
    }
    ImGui::Separator();

    auto entities = registry_->getEntities();
    for (auto entity : entities) {
      drawEntityNode(entity);
    }

    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
      selected_entity_ = std::nullopt;
    }

    // Blank space context menu
    if (ImGui::BeginPopupContextWindow("##SceneContext",
                                       ImGuiPopupFlags_MouseButtonRight |
                                           ImGuiPopupFlags_NoOpenOverItems)) {
      if (ImGui::MenuItem("Create Empty Entity")) {
        ECS::Entity new_ent = registry_->createEntity();
        registry_->addComponent<ECS::Components::Transform>(
            new_ent, ECS::Components::Transform{
                         glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                         glm::vec3(1.0f)});
        selected_entity_ = new_ent;
      }
      ImGui::EndPopup();
    }
  }

  ImGui::End();
}

void ScenePanel::drawEntityNode(ECS::Entity entity) {
  // Simple tag for now, in a real game we'd have a TagComponent
  char label[64];
  snprintf(label, sizeof(label), "Entity %u", entity);

  ImGuiTreeNodeFlags flags =
      ((selected_entity_ == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
      ImGuiTreeNodeFlags_OpenOnArrow;
  flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

  bool opened =
      ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags, "%s", label);

  if (ImGui::IsItemClicked()) {
    selected_entity_ = entity;
  }

  bool entityDeleted = false;
  if (ImGui::BeginPopupContextItem()) {
    if (ImGui::MenuItem("Destroy Entity")) {
      entityDeleted = true;
    }
    ImGui::EndPopup();
  }

  if (opened) {
    ImGui::TreePop();
  }

  if (entityDeleted) {
    registry_->destroyEntity(entity);
    if (selected_entity_ == entity) {
      selected_entity_ = std::nullopt;
    }
  }
}

} // namespace ParticleGL::UI

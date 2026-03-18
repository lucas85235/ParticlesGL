#include "MaterialsPanel.hpp"
#include "../core/AssetManager.hpp"
#include "../ecs/components/Renderable.hpp"
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

namespace ParticleGL::UI {

void MaterialsPanel::onImGuiRender() {
  ImGui::Begin("Materials");

  // Create new material
  ImGui::InputText("Name", new_material_name_, sizeof(new_material_name_));
  ImGui::SameLine();
  if (ImGui::Button("Create")) {
    std::string name(new_material_name_);
    if (!name.empty() && !Core::AssetManager::getMaterials().count(name)) {
      auto defaultMat = Core::AssetManager::getDefaultMaterial();
      auto newMat = std::make_shared<Renderer::Material>(
          name, defaultMat->shaderId, defaultMat->shader);
      newMat->baseColor = defaultMat->baseColor;
      Core::AssetManager::addMaterial(name, newMat);
      selected_material_id_ = name;
      new_material_name_[0] = '\0'; // Clear input
    }
  }

  ImGui::Separator();

  // List materials
  ImGui::BeginChild("MaterialList", ImVec2(150, 0), true);
  for (const auto &[name, material] : Core::AssetManager::getMaterials()) {
    bool isSelected = (selected_material_id_ == name);
    if (ImGui::Selectable(name.c_str(), isSelected)) {
      selected_material_id_ = name;
    }
  }
  ImGui::EndChild();

  ImGui::SameLine();

  // Edit selected material
  ImGui::BeginChild("MaterialEditor", ImVec2(0, 0), false);
  if (!selected_material_id_.empty() &&
      Core::AssetManager::getMaterials().count(selected_material_id_)) {
    auto mat = Core::AssetManager::getMaterial(selected_material_id_);
    ImGui::Text("Editing: %s", mat->id.c_str());
    ImGui::Separator();

    // Shader ID
    char shaderIdBuf[256];
    strncpy(shaderIdBuf, mat->shaderId.c_str(), sizeof(shaderIdBuf));
    if (ImGui::InputText("Shader ID", shaderIdBuf, sizeof(shaderIdBuf))) {
      mat->shaderId = shaderIdBuf;
      auto newShader = Core::AssetManager::getShader(mat->shaderId);
      if (newShader) {
        mat->shader = newShader;
      }
    }

    ImGui::ColorEdit4("Base Color", glm::value_ptr(mat->baseColor));

    ImGui::Spacing();
    if (registry_ && selected_entity_ &&
        registry_->hasComponent<ECS::Components::Renderable>(
            *selected_entity_)) {
      if (ImGui::Button("Assign to Selected Entity")) {
        auto &renderable = registry_->getComponent<ECS::Components::Renderable>(
            *selected_entity_);
        renderable.materialId = selected_material_id_;
      }
    }
  } else {
    ImGui::TextDisabled("Select a material to edit.");
  }
  ImGui::EndChild();

  ImGui::End();
}

} // namespace ParticleGL::UI

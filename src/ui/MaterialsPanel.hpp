#pragma once

#include "../ecs/Registry.hpp"
#include <optional>
#include <string>

namespace ParticleGL::UI {

class MaterialsPanel {
public:
  MaterialsPanel() = default;
  ~MaterialsPanel() = default;

  void setRegistry(ECS::Registry *registry) { registry_ = registry; }
  void setSelectedEntity(std::optional<ECS::Entity> entity) {
    selected_entity_ = entity;
  }

  void onImGuiRender();

private:
  ECS::Registry *registry_ = nullptr;
  std::optional<ECS::Entity> selected_entity_;

  std::string selected_material_id_;
  char new_material_name_[256] = "";
};

} // namespace ParticleGL::UI

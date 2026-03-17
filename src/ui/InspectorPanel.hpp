#pragma once

#include "../ecs/Registry.hpp"
#include <optional>

namespace ParticleGL::UI {

class InspectorPanel {
public:
  InspectorPanel() = default;
  ~InspectorPanel() = default;

  void setRegistry(ECS::Registry *registry) { registry_ = registry; }
  void setSelectedEntity(std::optional<ECS::Entity> entity) {
    selected_entity_ = entity;
  }

  void onImGuiRender();

private:
  ECS::Registry *registry_ = nullptr;
  std::optional<ECS::Entity> selected_entity_;
};

} // namespace ParticleGL::UI

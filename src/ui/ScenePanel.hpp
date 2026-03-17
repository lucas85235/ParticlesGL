#pragma once

#include "../ecs/Registry.hpp"
#include <optional>

namespace ParticleGL::UI {

class ScenePanel {
public:
  ScenePanel() = default;
  ~ScenePanel() = default;

  void setRegistry(ECS::Registry *registry) { registry_ = registry; }
  void onImGuiRender();

  std::optional<ECS::Entity> getSelectedEntity() const {
    return selected_entity_;
  }

private:
  ECS::Registry *registry_ = nullptr;
  std::optional<ECS::Entity> selected_entity_;

  void drawEntityNode(ECS::Entity entity);
};

} // namespace ParticleGL::UI

#pragma once

#include "../ecs/Registry.hpp"

namespace ParticleGL::UI {

class StatsPanel {
public:
  StatsPanel() = default;
  ~StatsPanel() = default;

  void setRegistry(ECS::Registry *registry) { registry_ = registry; }
  void onImGuiRender();

private:
  ECS::Registry *registry_ = nullptr;
};

} // namespace ParticleGL::UI

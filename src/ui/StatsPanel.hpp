#pragma once

#include "../ecs/Registry.hpp"
#include <glm/glm.hpp>

namespace ParticleGL::UI {

class StatsPanel {
public:
  StatsPanel() = default;
  ~StatsPanel() = default;

  void setRegistry(ECS::Registry *registry) { registry_ = registry; }
  void setComputeSampleColor(const glm::vec4 &color) {
    compute_sample_color_ = color;
  }
  void onImGuiRender();

private:
  ECS::Registry *registry_ = nullptr;
  glm::vec4 compute_sample_color_{0.0f};
};

} // namespace ParticleGL::UI

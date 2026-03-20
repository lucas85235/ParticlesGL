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
  void setGpuTimeMs(float time) { gpu_time_ms_ = time; }
  void setCpuTimeMs(float time) { cpu_time_ms_ = time; }
  void setDrawCalls(uint32_t count) { draw_calls_ = count; }

  void onImGuiRender();

private:
  ECS::Registry *registry_ = nullptr;
  glm::vec4 compute_sample_color_{0.0f};
  float gpu_time_ms_ = 0.0f;
  float cpu_time_ms_ = 0.0f;
  uint32_t draw_calls_ = 0;
};

} // namespace ParticleGL::UI

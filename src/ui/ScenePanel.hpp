#pragma once

#include "../ecs/Registry.hpp"
#include <optional>

namespace ParticleGL::Renderer { class GpuParticleBuffer; }

namespace ParticleGL::UI {

class ScenePanel {
public:
  ScenePanel() = default;
  ~ScenePanel() = default;

  void setRegistry(ECS::Registry *registry) { registry_ = registry; }
  void setGpuBuffer(Renderer::GpuParticleBuffer *gpuBuf) { gpu_buffer_ = gpuBuf; }
  void onImGuiRender();

  std::optional<ECS::Entity> getSelectedEntity() const {
    return selected_entity_;
  }

private:
  ECS::Registry *registry_ = nullptr;
  Renderer::GpuParticleBuffer *gpu_buffer_ = nullptr;
  std::optional<ECS::Entity> selected_entity_;

  void drawEntityNode(ECS::Entity entity);
};

} // namespace ParticleGL::UI

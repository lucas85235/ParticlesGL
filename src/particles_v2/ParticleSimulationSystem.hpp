#pragma once

#include "../ecs/System.hpp"
#include "renderer/ComputeShader.hpp"
#include "renderer/GpuParticleBuffer.hpp"

#include <glm/glm.hpp>
#include <memory>

namespace ParticleGL::ECS::Systems {

class ParticleSimulationSystem : public System {
public:
  ParticleSimulationSystem();
  ~ParticleSimulationSystem() override = default;

  void update(Registry &registry, float dt) override;

  void setGpuBuffer(Renderer::GpuParticleBuffer *buf) { gpu_buffer_ = buf; }

  // Phase 5: feed the View+Projection matrices for depth-collision math
  void setCamera(const glm::mat4 &view, const glm::mat4 &proj) {
    view_ = view;
    proj_ = proj;
  }

  // Phase 5: bind the depth pre-pass texture (OpenGL texture ID)
  void setSceneDepthTexture(uint32_t texId) { scene_depth_texture_ = texId; }

private:
  std::shared_ptr<Renderer::ComputeShader> simulate_shader_;
  std::shared_ptr<Renderer::ComputeShader> compact_shader_;

  bool initialized_ = false;
  void lazyInit();

  Renderer::GpuParticleBuffer *gpu_buffer_ = nullptr;

  // Phase 5 state
  float    elapsed_time_        = 0.0f;
  glm::mat4 view_               = glm::mat4(1.0f);
  glm::mat4 proj_               = glm::mat4(1.0f);
  uint32_t  scene_depth_texture_ = 0; // 0 = no depth collision
};

} // namespace ParticleGL::ECS::Systems

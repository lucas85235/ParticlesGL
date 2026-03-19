#pragma once

#include "../ecs/Registry.hpp"
#include "../renderer/Shader.hpp"
#include "renderer/GpuParticleBuffer.hpp"

namespace ParticleGL::ECS::Systems {

// Render-only system — not a simulation System, does not inherit System base.
class ParticleRenderSystem {
public:
  ParticleRenderSystem() = default;
  ~ParticleRenderSystem() = default;

  // Render all active particle pools to the currently bound framebuffer
  void render(Registry &registry, Renderer::Shader &particleShader);

  void setGpuBuffer(Renderer::GpuParticleBuffer* buf) { gpu_buffer_ = buf; }

private:
  // Expose the GPU timer result
  float getLastGpuTimeMs() const { return last_gpu_time_ms_; }

  void initGpuTimer();

  float last_gpu_time_ms_ = 0.0f;
  unsigned int query_id_ = 0;
  bool query_in_flight_ = false;

  Renderer::GpuParticleBuffer* gpu_buffer_ = nullptr;
};

} // namespace ParticleGL::ECS::Systems

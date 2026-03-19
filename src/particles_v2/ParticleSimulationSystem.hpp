#pragma once

#include "../ecs/System.hpp"
#include "renderer/ComputeShader.hpp"
#include "renderer/GpuParticleBuffer.hpp"

#include <memory>

namespace ParticleGL::ECS::Systems {

// Phase 3: GPU-driven simulation.
// Spawns new particles via glBufferSubData (the only CPU→GPU path),
// then dispatches particle_simulate.comp and particle_compact.comp.
class ParticleSimulationSystem : public System {
public:
  ParticleSimulationSystem();
  ~ParticleSimulationSystem() override = default;

  void update(Registry &registry, float dt) override;
  
  void setGpuBuffer(Renderer::GpuParticleBuffer* buf) { gpu_buffer_ = buf; }

private:
  std::shared_ptr<Renderer::ComputeShader> simulate_shader_;
  std::shared_ptr<Renderer::ComputeShader> compact_shader_;

  bool initialized_ = false;
  void lazyInit();

  Renderer::GpuParticleBuffer* gpu_buffer_ = nullptr;
};

} // namespace ParticleGL::ECS::Systems

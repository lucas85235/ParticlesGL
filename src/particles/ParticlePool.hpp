// DEPRECATED: use particles_v2/ParticlePoolComponent.hpp instead.
// This file is kept only to compile existing tests without modification.
#pragma once

#include "ParticleData.hpp"
#include "renderer/InstanceBuffer.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace ParticleGL::Particles {

class ParticlePool_Deprecated {
public:
  ParticlePool_Deprecated(uint32_t maxParticles);
  ~ParticlePool_Deprecated() = default;

  ParticlePool_Deprecated(const ParticlePool_Deprecated &) = delete;
  ParticlePool_Deprecated &operator=(const ParticlePool_Deprecated &) = delete;

  ParticlePool_Deprecated(ParticlePool_Deprecated &&) noexcept = default;
  ParticlePool_Deprecated &
  operator=(ParticlePool_Deprecated &&) noexcept = default;

  bool emit(const ParticleInstanceData &initialInstanceData,
            const ParticleSimData &initialSimData);

  void kill(uint32_t index);

  void flushToGPU();

  uint32_t getActiveParticleCount() const { return active_count_; }
  uint32_t getMaxParticles() const { return max_particles_; }

  ParticleInstanceData &getInstanceData(uint32_t index);
  ParticleSimData &getSimData(uint32_t index);

  const Renderer::InstanceBuffer &getInstanceBuffer() const {
    return *instance_buffer_;
  }

private:
  uint32_t max_particles_;
  uint32_t active_count_ = 0;

  std::vector<ParticleInstanceData> instance_data_;
  std::vector<ParticleSimData> sim_data_;

  std::unique_ptr<Renderer::InstanceBuffer> instance_buffer_;
};

// Legacy alias
using ParticlePool = ParticlePool_Deprecated;

} // namespace ParticleGL::Particles

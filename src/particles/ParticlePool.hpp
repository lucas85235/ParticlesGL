#pragma once

#include "ParticleData.hpp"
#include "renderer/InstanceBuffer.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace ParticleGL::Particles {

class ParticlePool {
public:
  ParticlePool(uint32_t maxParticles);
  ~ParticlePool() = default;

  // Prevent copies
  ParticlePool(const ParticlePool &) = delete;
  ParticlePool &operator=(const ParticlePool &) = delete;

  // Allow moves
  ParticlePool(ParticlePool &&) noexcept = default;
  ParticlePool &operator=(ParticlePool &&) noexcept = default;

  // Emits a new particle if we haven't reached capacity, returning false
  // otherwise
  bool emit(const ParticleInstanceData &initialInstanceData,
            const ParticleSimData &initialSimData);

  // Kills a particle by swapping it with the last active particle
  void kill(uint32_t index);

  // Flushes current active instance data to the linked InstanceBuffer
  void flushToGPU();

  // Accessors
  uint32_t getActiveParticleCount() const { return active_count_; }
  uint32_t getMaxParticles() const { return max_particles_; }

  ParticleInstanceData &getInstanceData(uint32_t index);
  ParticleSimData &getSimData(uint32_t index);

  // Retrieves the encapsulated OpenGL buffer
  const Renderer::InstanceBuffer &getInstanceBuffer() const {
    return *instance_buffer_;
  }

private:
  uint32_t max_particles_;
  uint32_t active_count_ = 0;

  // Strict contiguous arrays for cache locality
  std::vector<ParticleInstanceData> instance_data_;
  std::vector<ParticleSimData> sim_data_;

  // Wrapper covering the OpenGL GL_DYNAMIC_DRAW VBO
  std::unique_ptr<Renderer::InstanceBuffer> instance_buffer_;
};

} // namespace ParticleGL::Particles

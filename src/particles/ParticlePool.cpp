#include "ParticlePool.hpp"
#include "core/Logger.hpp"

namespace ParticleGL::Particles {

ParticlePool::ParticlePool(uint32_t maxParticles)
    : max_particles_(maxParticles) {

  instance_data_.resize(maxParticles);
  sim_data_.resize(maxParticles);

  // Define memory layout matching ParticleInstanceData struct bounds:
  // vec3 position, float scale, vec4 color -> 3 + 1 + 4 = 8 floats total (32
  // bytes stride)
  std::vector<uint32_t> layout = {3, 1, 4};

  // Allocate GPU buffer using max size cap
  instance_buffer_ = std::make_unique<Renderer::InstanceBuffer>(
      maxParticles * sizeof(ParticleInstanceData), layout);
}

bool ParticlePool::emit(const ParticleInstanceData &initialInstanceData,
                        const ParticleSimData &initialSimData) {
  if (active_count_ >= max_particles_) {
    return false; // Pool is full
  }

  // Insert at the end of the active slice
  instance_data_[active_count_] = initialInstanceData;
  sim_data_[active_count_] = initialSimData;

  active_count_++;
  return true;
}

void ParticlePool::kill(uint32_t index) {
  if (index >= active_count_) {
    PGL_ERROR("Attempted to kill particle outside active range");
    return;
  }

  // Rather than shifting elements, swap the dead element with the final active
  // element This turns a O(N) deletion into an O(1) op, destroying ordering but
  // particles don't need sorting yet.
  uint32_t lastActiveIndex = active_count_ - 1;

  if (index != lastActiveIndex) {
    instance_data_[index] = instance_data_[lastActiveIndex];
    sim_data_[index] = sim_data_[lastActiveIndex];
  }

  active_count_--;
}

void ParticlePool::flushToGPU() {
  if (active_count_ == 0)
    return;

  instance_buffer_->updateData(instance_data_.data(), active_count_);
}

ParticleInstanceData &ParticlePool::getInstanceData(uint32_t index) {
  // Note: No bounds check for performance! Caller is expected to iterate safely
  // within < active_count_
  return instance_data_[index];
}

ParticleSimData &ParticlePool::getSimData(uint32_t index) {
  // Note: No bounds check for performance!
  return sim_data_[index];
}

} // namespace ParticleGL::Particles

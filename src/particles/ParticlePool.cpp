// DEPRECATED: use particles_v2/ParticlePoolComponent.hpp instead.
#include "ParticlePool.hpp"
#include "core/Logger.hpp"

namespace ParticleGL::Particles {

ParticlePool_Deprecated::ParticlePool_Deprecated(uint32_t maxParticles)
    : max_particles_(maxParticles) {

  instance_data_.resize(maxParticles);
  sim_data_.resize(maxParticles);

  // Layout matches ParticleInstanceData: vec3 pos, float scale, vec4 color
  // -> 3 + 1 + 4 = 8 floats (32 bytes stride)
  std::vector<uint32_t> layout = {3, 1, 4};

  instance_buffer_ =
      std::make_unique<Renderer::InstanceBuffer>(maxParticles, layout);
}

bool ParticlePool_Deprecated::emit(
    const ParticleInstanceData &initialInstanceData,
    const ParticleSimData &initialSimData) {
  if (active_count_ >= max_particles_) {
    return false;
  }
  instance_data_[active_count_] = initialInstanceData;
  sim_data_[active_count_] = initialSimData;
  active_count_++;
  return true;
}

void ParticlePool_Deprecated::kill(uint32_t index) {
  if (index >= active_count_) {
    PGL_ERROR("Attempted to kill particle outside active range");
    return;
  }

  uint32_t lastActiveIndex = active_count_ - 1;
  if (index != lastActiveIndex) {
    instance_data_[index] = instance_data_[lastActiveIndex];
    sim_data_[index] = sim_data_[lastActiveIndex];
  }
  active_count_--;
}

void ParticlePool_Deprecated::flushToGPU() {
  if (active_count_ == 0)
    return;
  instance_buffer_->updateData(instance_data_.data(), active_count_);
}

ParticleInstanceData &ParticlePool_Deprecated::getInstanceData(uint32_t index) {
  return instance_data_[index];
}

ParticleSimData &ParticlePool_Deprecated::getSimData(uint32_t index) {
  return sim_data_[index];
}

} // namespace ParticleGL::Particles

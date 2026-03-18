#pragma once

#include "renderer/PersistentInstanceBuffer.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace ParticleGL::Particles {

// ECS component — SoA particle pool with orphaning + unsynchronized-map GPU
// upload.
//
// All simulation data lives on the CPU and is the canonical source of truth.
// Each frame, the GPU buffer is orphaned (new backing memory) and filled from
// the CPU arrays — no delta-reads through the mapped pointer.
//
// CPU arrays: position, velocity, life, maxLife, scale, color
// GPU buffer: write-only per frame via MappedInstanceBuffer::beginWrite()
struct ParticlePoolComponent {
  // ── CPU simulation data (canonical) ─────────────────────────────────────
  std::vector<glm::vec3> position;
  std::vector<glm::vec3> velocity;
  std::vector<float> life;
  std::vector<float> maxLife;
  std::vector<float> scale;
  std::vector<glm::vec4> color;

  // ── Pool control ─────────────────────────────────────────────────────────
  uint32_t active_count = 0;
  uint32_t max_count = 0;
  float emission_accumulator = 0.0f;

  // Non-owning — owned by the application layer
  Renderer::PersistentInstanceBuffer *gpu_buffer = nullptr;

  void init(uint32_t maxParticles) {
    max_count = maxParticles;
    position.assign(maxParticles, glm::vec3{0.0f});
    velocity.assign(maxParticles, glm::vec3{0.0f});
    life.assign(maxParticles, 0.0f);
    maxLife.assign(maxParticles, 1.0f);
    scale.assign(maxParticles, 1.0f);
    color.assign(maxParticles, glm::vec4{1.0f});
    active_count = 0;
    emission_accumulator = 0.0f;
  }

  // O(1) emit — appends to the active slice.
  bool emit(glm::vec3 pos, glm::vec3 vel, glm::vec4 startColor,
            float initialScale, float lifeDuration) {
    if (active_count >= max_count)
      return false;
    const uint32_t i = active_count;
    position[i] = pos;
    velocity[i] = vel;
    life[i] = 0.0f;
    maxLife[i] = lifeDuration;
    scale[i] = initialScale;
    color[i] = startColor;
    ++active_count;
    return true;
  }

  // O(1) kill — swap-and-decrement across all CPU arrays simultaneously.
  void kill(uint32_t index) {
    if (index >= active_count)
      return;
    const uint32_t last = active_count - 1;
    if (index != last) {
      position[index] = position[last];
      velocity[index] = velocity[last];
      life[index] = life[last];
      maxLife[index] = maxLife[last];
      scale[index] = scale[last];
      color[index] = color[last];
    }
    --active_count;
  }
};

} // namespace ParticleGL::Particles

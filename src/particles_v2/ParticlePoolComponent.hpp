#pragma once

#include "renderer/InstanceBuffer.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace ParticleGL::Particles {

// ECS component — SoA particle pool.
// Lifecycle is managed by the Registry (auto-destroyed with its entity).
// gpu_buffer is a non-owning pointer; the InstanceBuffer is created and owned
// by the application layer before being linked here.
struct ParticlePoolComponent {
  // ── CPU-only simulation data ────────────────────────────────────────────
  // These arrays are never uploaded to the GPU.
  std::vector<glm::vec3> velocity;
  std::vector<float> life;
  std::vector<float> maxLife;

  // ── GPU instance data ───────────────────────────────────────────────────
  // Written by CPU simulation, read by vertex shader via instancing.
  std::vector<glm::vec3> position;
  std::vector<float> scale;
  std::vector<glm::vec4> color;

  // ── Pool control ────────────────────────────────────────────────────────
  uint32_t active_count = 0;
  uint32_t max_count = 0;
  float emission_accumulator = 0.0f;

  // Non-owning handle — created and destroyed by the application/render system
  Renderer::InstanceBuffer *gpu_buffer = nullptr;

  // Pre-allocate all SoA arrays to max_count slots.
  void init(uint32_t maxParticles) {
    max_count = maxParticles;
    velocity.resize(maxParticles, glm::vec3{0.0f});
    life.resize(maxParticles, 0.0f);
    maxLife.resize(maxParticles, 1.0f);
    position.resize(maxParticles, glm::vec3{0.0f});
    scale.resize(maxParticles, 1.0f);
    color.resize(maxParticles, glm::vec4{1.0f});
    active_count = 0;
    emission_accumulator = 0.0f;
  }

  // O(1) emit — appends to the active slice.
  // Returns false and discards the spawn if the pool is full.
  bool emit(glm::vec3 pos, glm::vec3 vel, glm::vec4 startColor,
            float initialScale, float lifeDuration) {
    if (active_count >= max_count)
      return false;

    uint32_t i = active_count;
    position[i] = pos;
    velocity[i] = vel;
    color[i] = startColor;
    scale[i] = initialScale;
    life[i] = 0.0f;
    maxLife[i] = lifeDuration;
    ++active_count;
    return true;
  }

  // O(1) kill — swap-and-decrement over ALL SoA arrays simultaneously.
  void kill(uint32_t index) {
    if (index >= active_count)
      return;
    uint32_t last = active_count - 1;
    if (index != last) {
      position[index] = position[last];
      velocity[index] = velocity[last];
      color[index] = color[last];
      scale[index] = scale[last];
      life[index] = life[last];
      maxLife[index] = maxLife[last];
    }
    --active_count;
  }
};

} // namespace ParticleGL::Particles

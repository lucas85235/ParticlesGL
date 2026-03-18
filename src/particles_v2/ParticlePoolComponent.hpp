#pragma once

#include "renderer/PersistentInstanceBuffer.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace ParticleGL::Particles {

// ECS component — SoA particle pool with zero-copy GPU upload.
//
// CPU-only simulation data stays in std::vector (velocity, life, maxLife).
// GPU-visible data (position, scale, color) is written directly through the
// persistent mapped pointers in gpu_buffer — no local copies, no
// glBufferSubData.
//
// Lifecycle: gpu_buffer is a non-owning pointer owned by the application layer.
struct ParticlePoolComponent {
  // ── CPU-only simulation data ────────────────────────────────────────────
  std::vector<glm::vec3> velocity;
  std::vector<float> life;
  std::vector<float> maxLife;

  // ── Pool control ────────────────────────────────────────────────────────
  uint32_t active_count = 0;
  uint32_t max_count = 0;
  float emission_accumulator = 0.0f;

  // Non-owning — created and owned by the application layer
  Renderer::PersistentInstanceBuffer *gpu_buffer = nullptr;

  // Pre-allocate all CPU-side SoA arrays.
  void init(uint32_t maxParticles) {
    max_count = maxParticles;
    velocity.assign(maxParticles, glm::vec3{0.0f});
    life.assign(maxParticles, 0.0f);
    maxLife.assign(maxParticles, 1.0f);
    active_count = 0;
    emission_accumulator = 0.0f;
  }

  // O(1) emit — appends to the active slice and writes GPU data directly.
  // Returns false (silent discard) when the pool is full.
  bool emit(glm::vec3 pos, glm::vec3 vel, glm::vec4 startColor,
            float initialScale, float lifeDuration) {
    if (active_count >= max_count)
      return false;

    const uint32_t i = active_count;

    // CPU data
    velocity[i] = vel;
    life[i] = 0.0f;
    maxLife[i] = lifeDuration;

    // GPU data — written directly into persistently mapped VRAM
    if (gpu_buffer) {
      gpu_buffer->positionPtr()[i] = pos;
      gpu_buffer->scalePtr()[i] = initialScale;
      gpu_buffer->colorPtr()[i] = startColor;
    }

    ++active_count;
    return true;
  }

  // O(1) kill — swap-and-decrement across CPU and GPU arrays simultaneously.
  void kill(uint32_t index) {
    if (index >= active_count)
      return;
    const uint32_t last = active_count - 1;

    if (index != last) {
      // CPU arrays
      velocity[index] = velocity[last];
      life[index] = life[last];
      maxLife[index] = maxLife[last];

      // GPU arrays — direct pointer write into VRAM
      if (gpu_buffer) {
        gpu_buffer->positionPtr()[index] = gpu_buffer->positionPtr()[last];
        gpu_buffer->scalePtr()[index] = gpu_buffer->scalePtr()[last];
        gpu_buffer->colorPtr()[index] = gpu_buffer->colorPtr()[last];
      }
    }

    --active_count;
  }
};

} // namespace ParticleGL::Particles

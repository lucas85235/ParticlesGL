#pragma once

#include "renderer/GpuParticleBuffer.hpp"

#include <cstdint>

namespace ParticleGL::Particles {

// ECS component for the Phase 3 GPU-driven particle pool.
//
// All simulation data lives on the GPU in GpuParticleBuffer SSBOs.
// The CPU only tracks:
//   active_count    — read-back of GPU atomic (for InspectorPanel)
//   max_count       — pool capacity
//   spawn backlog   — accumulator for sub-frame emission
//   gpu_buffer      — non-owning pointer to the GPU resource block
struct ParticlePoolComponent {
  uint32_t active_count = 0;
  uint32_t max_count = 0;
  float emission_accumulator = 0.0f;

  // Non-owning — owned by the application layer
  Renderer::GpuParticleBuffer *gpu_buffer = nullptr;

  void init(uint32_t maxParticles, Renderer::GpuParticleBuffer *buf) {
    max_count = maxParticles;
    active_count = 0;
    emission_accumulator = 0.0f;
    gpu_buffer = buf;
    if (buf)
      buf->setActiveCount(0);
  }
};

} // namespace ParticleGL::Particles

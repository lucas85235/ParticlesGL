#pragma once

#include <cstdint>

namespace ParticleGL::ECS::Components {

// Phase 4: Global Pool + Multi-Emitter
// This component no longer owns a GpuParticleBuffer. It just holds the metadata
// for its slice of the Global GPU Pool.
struct ParticlePoolComponent {
  uint32_t emitter_index = 0; // Index in the global counters/draw arrays
  uint32_t pool_offset = 0;   // Start index in the global SSBOs
  uint32_t pool_size = 0;     // Maximum particles for this emitter
  
  // Read back asynchronously from GPU (or kept synced via ECS)
  uint32_t active_count = 0;

  float emission_accumulator = 0.0f;
};

} // namespace ParticleGL::ECS::Components

#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace ParticleGL::Renderer {

// Manages the complete set of GPU-side resources for the Phase 3 particle
// system.
//
// SSBO layout (one entry per particle slot):
//   binding 0: positions   vec4[maxParticles]   — xyz used, w unused
//   binding 1: velocities  vec4[maxParticles]   — xyz used, w unused
//   binding 2: lives       float[maxParticles]
//   binding 3: maxLives    float[maxParticles]
//   binding 4: colors      vec4[maxParticles]   — current (lerped) color
//   binding 5: startColors vec4[maxParticles]   — spawn color
//   binding 6: endColors   vec4[maxParticles]   — death color
//   binding 7: scales      float[maxParticles]
//   binding 8: killList    uint[maxParticles]
//   binding 9: drawCmd     DrawElementsIndirectCommand
//
// Atomic counter buffers (separate from SSBOs):
//   killCount  binding 0 — incremented by simulate, reset by compact
//   activeCount binding 1 — authoritative particle count on GPU
class GpuParticleBuffer {
public:
  explicit GpuParticleBuffer(uint32_t maxParticles);
  ~GpuParticleBuffer();

  GpuParticleBuffer(const GpuParticleBuffer &) = delete;
  GpuParticleBuffer &operator=(const GpuParticleBuffer &) = delete;

  // Bind all SSBOs to their respective binding points.
  void bindSsbos() const;

  // ── Atomic-style counters via SSBO (GL 4.3 compatible) ────────────────────
  // layout: [0]=killCount, [1]=activeCount (both uint32)
  // atomicCounterExchange requires GLSL 4.6; atomicAdd on SSBO uint is 4.3
  // core.
  void bindCounterSsbo() const;

  // Bind the draw indirect buffer to GL_DRAW_INDIRECT_BUFFER.
  void bindDrawIndirect() const;

  // Write spawn data for `count` new particles starting at `offset`.
  // This is the only CPU→GPU path in Phase 3.
  struct SpawnData {
    glm::vec4 position; // xyz = position, w = unused
    glm::vec4 velocity; // xyz = velocity, w = unused
    float life;
    float maxLife;
    glm::vec4 color; // initial (same as start)
    glm::vec4 startColor;
    glm::vec4 endColor;
    float scale;
  };
  void uploadSpawnBatch(uint32_t offset, const SpawnData *data, uint32_t count);

  // Initialize the draw command buffer: set index count, zero instanceCount.
  // Must be called once after the mesh is known.
  void initDrawCommand(uint32_t indexCount);

  // Set the active count on the GPU atomic counter (used at init or after pool
  // reset).
  void setActiveCount(uint32_t count);

  // Read active count from GPU (synchronous — only for debug / inspector use).
  uint32_t readActiveCount() const;

  uint32_t getMaxParticles() const { return max_particles_; }

private:
  uint32_t max_particles_ = 0;

  // SSBOs (bindings 0..9)
  uint32_t ssbo_positions_ = 0;
  uint32_t ssbo_velocities_ = 0;
  uint32_t ssbo_lives_ = 0;
  uint32_t ssbo_maxLives_ = 0;
  uint32_t ssbo_colors_ = 0;
  uint32_t ssbo_startColors_ = 0;
  uint32_t ssbo_endColors_ = 0;
  uint32_t ssbo_scales_ = 0;
  uint32_t ssbo_killList_ = 0;
  uint32_t ssbo_drawCmd_ = 0;  // also used as GL_DRAW_INDIRECT_BUFFER
  uint32_t ssbo_counters_ = 0; // binding 10: {uint killCount, uint activeCount}
};

} // namespace ParticleGL::Renderer

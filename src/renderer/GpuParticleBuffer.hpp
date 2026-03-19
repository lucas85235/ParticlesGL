#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace ParticleGL::Renderer {

// Phase 4: Global GPU-driven particle buffer accommodating multiple emitters.
class GpuParticleBuffer {
public:
  struct SpawnData {
    glm::vec4 position;
    glm::vec4 velocity;
    glm::vec4 color;
    glm::vec4 startColor;
    glm::vec4 endColor;
    float life;
    float maxLife;
    float scale;
  };

  GpuParticleBuffer(uint32_t maxTotalParticles, uint32_t maxEmitters);
  ~GpuParticleBuffer();

  void reset();

  // Allocates an emitter range from the global pool.
  // Returns true and populates outEmitterIndex and outPoolOffset if space allows.
  bool allocateEmitter(uint32_t max_particles, uint32_t &outEmitterIndex,
                       uint32_t &outPoolOffset);

  // Frees an allocated emitter slot. Note: Simple linear allocator
  // does not reuse fragmented memory in this POC, but it frees the emitter ID.
  void freeEmitter(uint32_t emitterIndex);

  void bindSsbos() const;
  void bindCounterSsbo() const;
  void bindDrawIndirect() const;

  // CPU->GPU upload of new particles within an emitter's range
  void uploadSpawnBatch(uint32_t poolOffset, uint32_t activeCountOffset,
                        const SpawnData *data, uint32_t count);

  // Initializes the multi-draw indirect command struct for the specified emitter
  void initDrawCommand(uint32_t emitterIndex, uint32_t indexCount,
                       uint32_t poolOffset);

  // Synchronizes the active count via the counters SSBO array
  void setActiveCount(uint32_t emitterIndex, uint32_t count);
  uint32_t readActiveCount(uint32_t emitterIndex) const;

  uint32_t getMaxEmitters() const { return max_emitters_; }

private:
  uint32_t max_total_particles_ = 0;
  uint32_t max_emitters_ = 0;

  // Linear allocator state
  uint32_t next_free_slot_ = 0;

  // ID management
  std::vector<bool> emitter_allocated_;

  // SSBO handles
  uint32_t ssbo_positions_ = 0;
  uint32_t ssbo_velocities_ = 0;
  uint32_t ssbo_lives_ = 0;
  uint32_t ssbo_maxLives_ = 0;
  uint32_t ssbo_colors_ = 0;
  uint32_t ssbo_startColors_ = 0;
  uint32_t ssbo_endColors_ = 0;
  uint32_t ssbo_scales_ = 0;
  uint32_t ssbo_killList_ = 0;

  // Multi-Emitter buffers (arrays of size max_emitters_)
  uint32_t ssbo_drawCmd_ = 0;  // Array of DrawElementsIndirectCommand
  uint32_t ssbo_counters_ = 0; // Array of {killCount, activeCount}
};

} // namespace ParticleGL::Renderer

#include "GpuParticleBuffer.hpp"
#include "core/Logger.hpp"

#include <cstring>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace ParticleGL::Renderer {

static uint32_t makeSSBO(uint32_t binding, GLsizeiptr bytes,
                         const void *initialData = nullptr) {
  GLuint buf = 0;
  glGenBuffers(1, &buf);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
  glBufferData(GL_SHADER_STORAGE_BUFFER, bytes, initialData, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buf);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  return buf;
}

GpuParticleBuffer::GpuParticleBuffer(uint32_t maxTotalParticles,
                                     uint32_t maxEmitters)
    : max_total_particles_(maxTotalParticles), max_emitters_(maxEmitters) {

  emitter_allocated_.resize(maxEmitters, false);

  const GLsizeiptr N4  = maxTotalParticles * sizeof(glm::vec4);
  const GLsizeiptr N1f = maxTotalParticles * sizeof(float);
  const GLsizeiptr N1u = maxTotalParticles * sizeof(uint32_t);

  ssbo_positions_   = makeSSBO(0, N4);
  ssbo_velocities_  = makeSSBO(1, N4);
  ssbo_lives_       = makeSSBO(2, N1f);
  ssbo_maxLives_    = makeSSBO(3, N1f);
  ssbo_colors_      = makeSSBO(4, N4);
  ssbo_startColors_ = makeSSBO(5, N4);
  ssbo_endColors_   = makeSSBO(6, N4);
  ssbo_scales_      = makeSSBO(7, N1f);
  ssbo_killList_    = makeSSBO(8, N1u);

  // Array of DrawElementsIndirectCommand (5 * uint32_t) per emitter.
  // Must be zero-initialized so unused emitters have count/instanceCount = 0.
  std::vector<uint32_t> initCmds(maxEmitters * 5, 0u);
  ssbo_drawCmd_ = makeSSBO(9, maxEmitters * 5 * sizeof(uint32_t), initCmds.data());

  // Array of {uint killCount, uint activeCount} per emitter
  std::vector<uint32_t> initCounters(maxEmitters * 2, 0u);
  ssbo_counters_ = makeSSBO(10, maxEmitters * 2 * sizeof(uint32_t), initCounters.data());

  // Phase 6: Radix Sort SSBOs
  // Sorted index buffers — initialised as identity range for first pass
  std::vector<uint32_t> initIndices(maxTotalParticles, 0u);
  for (uint32_t i = 0; i < maxTotalParticles; ++i) initIndices[i] = i;
  ssbo_sortedIndices_    = makeSSBO(11, N1u, initIndices.data());
  ssbo_sortKeys_         = makeSSBO(12, N1u);               // populated by radix_sort_init.comp
  ssbo_histogram_        = makeSSBO(13, 256 * sizeof(uint32_t)); // 256 buckets
  ssbo_prefix_           = makeSSBO(14, 256 * sizeof(uint32_t)); // exclusive prefix
  ssbo_sortedIndicesOut_ = makeSSBO(15, N1u);               // ping-pong target

  PGL_INFO("GpuParticleBuffer (Phase 4): allocated global pool with "
           << maxTotalParticles << " particles and " << maxEmitters
           << " emitters max.");
}

GpuParticleBuffer::~GpuParticleBuffer() {
  uint32_t bufs[] = {
      ssbo_positions_, ssbo_velocities_, ssbo_lives_,  ssbo_maxLives_,
      ssbo_colors_,    ssbo_startColors_, ssbo_endColors_, ssbo_scales_,
      ssbo_killList_,  ssbo_drawCmd_,    ssbo_counters_,
      // Phase 6 sort buffers
      ssbo_sortedIndices_, ssbo_sortKeys_, ssbo_histogram_,
      ssbo_prefix_,        ssbo_sortedIndicesOut_};
  glDeleteBuffers(16, bufs);
}

void GpuParticleBuffer::reset() {
  std::fill(emitter_allocated_.begin(), emitter_allocated_.end(), false);
  next_free_slot_ = 0;

  std::vector<uint32_t> initCmds(max_emitters_ * 5, 0u);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_drawCmd_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, initCmds.size() * sizeof(uint32_t), initCmds.data());

  std::vector<uint32_t> initCounters(max_emitters_ * 2, 0u);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_counters_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, initCounters.size() * sizeof(uint32_t), initCounters.data());

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

bool GpuParticleBuffer::allocateEmitter(uint32_t max_particles,
                                        uint32_t &outEmitterIndex,
                                        uint32_t &outPoolOffset) {
  // Find a free emitter ID
  uint32_t id = 0;
  bool found = false;
  for (uint32_t i = 0; i < max_emitters_; ++i) {
    if (!emitter_allocated_[i]) {
      id = i;
      found = true;
      break;
    }
  }

  if (!found) {
    PGL_ERROR("GpuParticleBuffer: No free emitter IDs available!");
    return false;
  }

  // Linear range allocator (bump allocator strategy)
  if (next_free_slot_ + max_particles > max_total_particles_) {
    PGL_ERROR("GpuParticleBuffer: Out of particle memory in global pool!");
    return false;
  }

  emitter_allocated_[id] = true;
  outEmitterIndex = id;
  outPoolOffset = next_free_slot_;
  
  next_free_slot_ += max_particles;
  return true;
}

void GpuParticleBuffer::freeEmitter(uint32_t emitterIndex) {
  if (emitterIndex < max_emitters_) {
    emitter_allocated_[emitterIndex] = false;
    // In this POC we use a bump allocator, so we only reclaim the ID, not the buffer space.
  }
}

void GpuParticleBuffer::bindSsbos() const {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_positions_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_velocities_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_lives_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_maxLives_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_colors_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_startColors_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_endColors_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_scales_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssbo_killList_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssbo_drawCmd_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssbo_counters_);
}

void GpuParticleBuffer::bindCounterSsbo() const {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssbo_counters_);
}

void GpuParticleBuffer::bindDrawIndirect() const {
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ssbo_drawCmd_);
}

void GpuParticleBuffer::bindSortSsbos() const {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, ssbo_sortedIndices_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, ssbo_sortKeys_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, ssbo_histogram_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, ssbo_prefix_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, ssbo_sortedIndicesOut_);
}

void GpuParticleBuffer::swapSortBuffers() {
  std::swap(ssbo_sortedIndices_, ssbo_sortedIndicesOut_);
  // Re-bind so shader reads the up-to-date assignment
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, ssbo_sortedIndices_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, ssbo_sortedIndicesOut_);
}

void GpuParticleBuffer::uploadSpawnBatch(uint32_t poolOffset,
                                         uint32_t activeCountOffset,
                                         const SpawnData *data,
                                         uint32_t count) {
  if (count == 0 || !data) return;

  uint32_t absoluteOffset = poolOffset + activeCountOffset;
  const GLintptr byteOffset4  = absoluteOffset * sizeof(glm::vec4);
  const GLintptr byteOffset1f = absoluteOffset * sizeof(float);

  std::vector<glm::vec4> pos(count), vel(count), col(count), sc(count), ec(count);
  std::vector<float>     life(count), maxLife(count), scale(count);

  for (uint32_t i = 0; i < count; ++i) {
    pos[i]     = data[i].position;
    vel[i]     = data[i].velocity;
    life[i]    = data[i].life;
    maxLife[i] = data[i].maxLife;
    col[i]     = data[i].color;
    sc[i]      = data[i].startColor;
    ec[i]      = data[i].endColor;
    scale[i]   = data[i].scale;
  }

  auto upload4 = [&](GLuint buf, const glm::vec4 *src) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, byteOffset4,
                    count * sizeof(glm::vec4), src);
  };
  auto upload1f = [&](GLuint buf, const float *src) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, byteOffset1f,
                    count * sizeof(float), src);
  };

  upload4(ssbo_positions_,   pos.data());
  upload4(ssbo_velocities_,  vel.data());
  upload1f(ssbo_lives_,      life.data());
  upload1f(ssbo_maxLives_,   maxLife.data());
  upload4(ssbo_colors_,      col.data());
  upload4(ssbo_startColors_, sc.data());
  upload4(ssbo_endColors_,   ec.data());
  upload1f(ssbo_scales_,     scale.data());

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GpuParticleBuffer::initDrawCommand(uint32_t emitterIndex,
                                        uint32_t indexCount,
                                        uint32_t poolOffset) {
  // Struct: count, instanceCount, firstIndex, baseVertex, baseInstance
  // baseInstance receives the poolOffset to shift gl_InstanceID in vertex shader via gl_BaseInstanceARB
  uint32_t cmd[5] = {indexCount, 0u, 0u, 0u, poolOffset};
  
  GLintptr byteOffset = emitterIndex * 5 * sizeof(uint32_t);
  
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_drawCmd_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, byteOffset, sizeof(cmd), cmd);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GpuParticleBuffer::setActiveCount(uint32_t emitterIndex, uint32_t count) {
  // Array of {killCount, activeCount}
  GLintptr byteOffset = (emitterIndex * 2 + 1) * sizeof(uint32_t);
  
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_counters_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, byteOffset, sizeof(uint32_t), &count);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

uint32_t GpuParticleBuffer::readActiveCount(uint32_t emitterIndex) const {
  uint32_t val = 0;
  GLintptr byteOffset = (emitterIndex * 2 + 1) * sizeof(uint32_t);
  
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_counters_);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, byteOffset, sizeof(uint32_t), &val);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  return val;
}

} // namespace ParticleGL::Renderer

#include "GpuParticleBuffer.hpp"
#include "core/Logger.hpp"

#include <cstring>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

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

GpuParticleBuffer::GpuParticleBuffer(uint32_t maxParticles)
    : max_particles_(maxParticles) {

  const GLsizeiptr N4 = maxParticles * sizeof(glm::vec4);
  const GLsizeiptr N1f = maxParticles * sizeof(float);
  const GLsizeiptr N1u = maxParticles * sizeof(uint32_t);

  ssbo_positions_ = makeSSBO(0, N4);
  ssbo_velocities_ = makeSSBO(1, N4);
  ssbo_lives_ = makeSSBO(2, N1f);
  ssbo_maxLives_ = makeSSBO(3, N1f);
  ssbo_colors_ = makeSSBO(4, N4);
  ssbo_startColors_ = makeSSBO(5, N4);
  ssbo_endColors_ = makeSSBO(6, N4);
  ssbo_scales_ = makeSSBO(7, N1f);
  ssbo_killList_ = makeSSBO(8, N1u);

  // DrawElementsIndirectCommand (5 × uint32)
  ssbo_drawCmd_ = makeSSBO(9, 5 * sizeof(uint32_t));

  // Counter SSBO at binding 10: { uint killCount, uint activeCount }
  // Both start at zero. Shaders use atomicAdd / direct write on these uints.
  uint32_t initCounters[2] = {0u, 0u};
  ssbo_counters_ = makeSSBO(10, 2 * sizeof(uint32_t), initCounters);

  PGL_INFO("GpuParticleBuffer: allocated "
           << maxParticles
           << " slots on GPU (10 SSBOs, GL 4.3 counter strategy)");
}

GpuParticleBuffer::~GpuParticleBuffer() {
  uint32_t bufs[] = {ssbo_positions_, ssbo_velocities_, ssbo_lives_,
                     ssbo_maxLives_,  ssbo_colors_,     ssbo_startColors_,
                     ssbo_endColors_, ssbo_scales_,     ssbo_killList_,
                     ssbo_drawCmd_,   ssbo_counters_};
  glDeleteBuffers(11, bufs);
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

void GpuParticleBuffer::uploadSpawnBatch(uint32_t offset, const SpawnData *data,
                                         uint32_t count) {
  if (count == 0 || !data)
    return;

  const GLintptr byteOffset4 = offset * sizeof(glm::vec4);
  const GLintptr byteOffset1f = offset * sizeof(float);

  std::vector<glm::vec4> pos(count), vel(count), col(count), sc(count),
      ec(count);
  std::vector<float> life(count), maxLife(count), scale(count);

  for (uint32_t i = 0; i < count; ++i) {
    pos[i] = data[i].position;
    vel[i] = data[i].velocity;
    life[i] = data[i].life;
    maxLife[i] = data[i].maxLife;
    col[i] = data[i].color;
    sc[i] = data[i].startColor;
    ec[i] = data[i].endColor;
    scale[i] = data[i].scale;
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

  upload4(ssbo_positions_, pos.data());
  upload4(ssbo_velocities_, vel.data());
  upload1f(ssbo_lives_, life.data());
  upload1f(ssbo_maxLives_, maxLife.data());
  upload4(ssbo_colors_, col.data());
  upload4(ssbo_startColors_, sc.data());
  upload4(ssbo_endColors_, ec.data());
  upload1f(ssbo_scales_, scale.data());

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GpuParticleBuffer::initDrawCommand(uint32_t indexCount) {
  uint32_t cmd[5] = {indexCount, 0u, 0u, 0u, 0u};
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_drawCmd_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(cmd), cmd);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GpuParticleBuffer::setActiveCount(uint32_t count) {
  // activeCount is at byte offset 4 (after killCount) in ssbo_counters_
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_counters_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), sizeof(uint32_t),
                  &count);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

uint32_t GpuParticleBuffer::readActiveCount() const {
  uint32_t val = 0;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_counters_);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t),
                     sizeof(uint32_t), &val);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  return val;
}

} // namespace ParticleGL::Renderer

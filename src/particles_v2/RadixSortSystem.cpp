#include "RadixSortSystem.hpp"
#include "core/Logger.hpp"

#include <glad/glad.h>

namespace ParticleGL::ECS::Systems {

void RadixSortSystem::lazyInit() {
  if (initialized_) return;

  init_shader_      = Renderer::ComputeShader::loadFromFile("assets/shaders/radix_sort_init.comp");
  histogram_shader_ = Renderer::ComputeShader::loadFromFile("assets/shaders/radix_sort_histogram.comp");
  scan_shader_      = Renderer::ComputeShader::loadFromFile("assets/shaders/radix_sort_scan.comp");
  scatter_shader_   = Renderer::ComputeShader::loadFromFile("assets/shaders/radix_sort_scatter.comp");

  if (!init_shader_ || !histogram_shader_ || !scan_shader_ || !scatter_shader_) {
    PGL_ERROR("RadixSortSystem: failed to load one or more sort compute shaders.");
    return;
  }

  PGL_INFO("RadixSortSystem: all sort shaders loaded successfully.");
  initialized_ = true;
}

void RadixSortSystem::sort(Registry &registry) {
  lazyInit();
  if (!initialized_ || !gpu_buffer_) return;

  using ECS::Components::ParticleBlendMode;
  auto &gpuBuf = *gpu_buffer_;

  auto emitters = registry.getEntitiesWith<ECS::Components::ParticleEmitter>();

  gpuBuf.bindSsbos();
  gpuBuf.bindSortSsbos();

  for (auto entity : emitters) {
    if (!registry.hasComponent<ECS::Components::ParticlePoolComponent>(entity)) continue;
    auto &emitter = registry.getComponent<ECS::Components::ParticleEmitter>(entity);

    // Only sort when using Alpha blending — Additive is order-independent.
    if (emitter.blendMode != ParticleBlendMode::Alpha) continue;

    auto &pool = registry.getComponent<ECS::Components::ParticlePoolComponent>(entity);
    if (pool.active_count == 0) continue;

    const uint32_t N = pool.active_count;
    const uint32_t groups256 = (N + 255u) / 256u;

    // ── Pass 0: build distance sort keys and reset identity index buffer ─────
    init_shader_->bind();
    init_shader_->setUInt("u_emitterIndex", pool.emitter_index);
    init_shader_->setUInt("u_poolOffset",   pool.pool_offset);
    init_shader_->setVec3("u_cameraPos",    camera_pos_);
    init_shader_->dispatch(groups256);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // ── 4 Radix passes × 8 bits = full 32-bit sort key ──────────────────────
    for (uint32_t pass = 0; pass < 4; ++pass) {
      const uint32_t bitShift = pass * 8u;

      // Zero the 256-bucket global histogram before each pass
      GLuint histogramBuf = gpuBuf.getHistogramSsbo();
      const uint32_t zero = 0u;
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuf);
      glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

      // ── 2a. Histogram pass ────────────────────────────────────────────────
      histogram_shader_->bind();
      histogram_shader_->setUInt("u_emitterIndex", pool.emitter_index);
      histogram_shader_->setUInt("u_poolOffset",   pool.pool_offset);
      histogram_shader_->setUInt("u_bitShift",     bitShift);
      histogram_shader_->dispatch(groups256);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      // ── 2b. Prefix sum (scan) — always 1 workgroup of 256 ────────────────
      scan_shader_->bind();
      scan_shader_->dispatch(1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      // ── 2c. Scatter ───────────────────────────────────────────────────────
      scatter_shader_->bind();
      scatter_shader_->setUInt("u_emitterIndex", pool.emitter_index);
      scatter_shader_->setUInt("u_poolOffset",   pool.pool_offset);
      scatter_shader_->setUInt("u_bitShift",     bitShift);
      scatter_shader_->dispatch(groups256);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      // Swap ping-pong buffers so next pass reads the output of this one
      gpuBuf.swapSortBuffers();
    }
  }
}

} // namespace ParticleGL::ECS::Systems

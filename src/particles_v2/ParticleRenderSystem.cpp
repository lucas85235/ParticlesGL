#include "ParticleRenderSystem.hpp"
#include "../core/AssetManager.hpp"
#include "../ecs/components/ParticleEmitter.hpp"
#include "../renderer/Renderer.hpp"
#include "ParticlePoolComponent.hpp"

#include <glad/glad.h>

namespace ParticleGL::ECS::Systems {

void ParticleRenderSystem::initGpuTimer() { glGenQueries(1, &query_id_); }

void ParticleRenderSystem::render(Registry &registry,
                                  Renderer::Shader &particleShader) {
  if (query_id_ == 0) {
    initGpuTimer();
  }

  // Async GPU timer — retrieve previous frame's result without stalling
  if (query_in_flight_) {
    GLuint available = 0;
    glGetQueryObjectuiv(query_id_, GL_QUERY_RESULT_AVAILABLE, &available);
    if (available) {
      GLuint64 elapsed_ns = 0;
      glGetQueryObjectui64v(query_id_, GL_QUERY_RESULT, &elapsed_ns);
      last_gpu_time_ms_ = static_cast<float>(elapsed_ns) / 1000000.0f;
      query_in_flight_ = false;
    }
  }

  if (!query_in_flight_) {
    glBeginQuery(GL_TIME_ELAPSED, query_id_);
  }

  if (!gpu_buffer_) {
    if (!query_in_flight_) {
      glEndQuery(GL_TIME_ELAPSED);
      query_in_flight_ = true;
    }
    return;
  }

  auto &gpuBuf = *gpu_buffer_;

  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE); // Particles don't write depth

  auto mesh = Core::AssetManager::getDefaultMesh();
  particleShader.bind();
  gpuBuf.bindSsbos();
  gpuBuf.bindDrawIndirect();

  using ECS::Components::ParticleBlendMode;
  auto emitters = registry.getEntitiesWith<ECS::Components::ParticleEmitter>();

  for (auto entity : emitters) {
    if (!registry.hasComponent<ECS::Components::ParticlePoolComponent>(entity)) {
      continue;
    }
    
    auto &pool = registry.getComponent<ECS::Components::ParticlePoolComponent>(entity);
    if (pool.pool_size == 0 || pool.active_count == 0) {
      continue;
    }

    auto &emitter = registry.getComponent<ECS::Components::ParticleEmitter>(entity);

    // Apply per-emitter blend mode
    if (emitter.blendMode == ParticleBlendMode::Additive) {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Order-independent, Additive
    } else {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard Alpha
    }

    // Byte offset into the indirect buffer array: each struct is 5 uint32_t (20 bytes)
    const void* indirectOffset = reinterpret_cast<const void*>(pool.emitter_index * 5 * sizeof(uint32_t));
    Renderer::Renderer::drawIndirect(*mesh, particleShader, indirectOffset);
  }

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);

  if (!query_in_flight_) {
    glEndQuery(GL_TIME_ELAPSED);
    query_in_flight_ = true;
  }
}

} // namespace ParticleGL::ECS::Systems

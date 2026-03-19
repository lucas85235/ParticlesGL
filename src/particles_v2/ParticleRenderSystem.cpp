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

  // Determine blend mode from the first emitter found (all emitters share the
  // same blend mode in this POC; choose the dominant one).
  using ECS::Components::ParticleBlendMode;
  ParticleBlendMode blendMode = ParticleBlendMode::Additive;
  auto emitters = registry.getEntitiesWith<ECS::Components::ParticleEmitter>();
  if (!emitters.empty()) {
    blendMode = registry.getComponent<ECS::Components::ParticleEmitter>(emitters[0]).blendMode;
  }

  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE); // Particles don't write depth
  if (blendMode == ParticleBlendMode::Additive) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Order-independent, no sort needed
  } else {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Correct alpha compositing
  }

  auto mesh = Core::AssetManager::getDefaultMesh();

  particleShader.bind();
  gpuBuf.bindSsbos();
  gpuBuf.bindDrawIndirect();

  Renderer::Renderer::drawMultiIndirect(*mesh, particleShader, gpuBuf.getMaxEmitters());

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);

  if (!query_in_flight_) {
    glEndQuery(GL_TIME_ELAPSED);
    query_in_flight_ = true;
  }
}

} // namespace ParticleGL::ECS::Systems

#include "ParticleRenderSystem.hpp"
#include "../core/AssetManager.hpp"
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

  // Phase 4: Single MultiDrawElementsIndirect covers ALL emitters!
  auto mesh = Core::AssetManager::getDefaultMesh();

  particleShader.bind();
  gpuBuf.bindSsbos();
  gpuBuf.bindDrawIndirect();

  Renderer::Renderer::drawMultiIndirect(*mesh, particleShader, gpuBuf.getMaxEmitters());

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

  if (!query_in_flight_) {
    glEndQuery(GL_TIME_ELAPSED);
    query_in_flight_ = true;
  }
}

} // namespace ParticleGL::ECS::Systems

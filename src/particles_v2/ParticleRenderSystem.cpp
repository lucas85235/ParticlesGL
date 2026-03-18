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

  auto pools = registry.getEntitiesWith<Particles::ParticlePoolComponent>();
  auto mesh = Core::AssetManager::getDefaultMesh();

  for (Entity entity : pools) {
    auto &pool =
        registry.getComponent<Particles::ParticlePoolComponent>(entity);

    if (!pool.gpu_buffer || pool.active_count == 0)
      continue;

    auto &gpuBuf = *pool.gpu_buffer;

    // Phase 3: no VBOs — the vertex shader reads per-instance data from SSBOs
    // via gl_InstanceID. We just bind the mesh, the SSBOs, and issue an
    // indirect draw (instanceCount comes from the GPU directly).
    particleShader.bind();
    gpuBuf.bindSsbos();
    gpuBuf.bindDrawIndirect();

    // Renderer::drawIndirect sets u_ViewProjection from the active camera
    // then issues glDrawElementsIndirect from the bound indirect buffer.
    Renderer::Renderer::drawIndirect(*mesh, particleShader);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
  }

  if (!query_in_flight_) {
    glEndQuery(GL_TIME_ELAPSED);
    query_in_flight_ = true;
  }
}

} // namespace ParticleGL::ECS::Systems

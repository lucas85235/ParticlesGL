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

  // Retrieve async timer result from previous frame (non-blocking)
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

    if (pool.active_count == 0 || pool.gpu_buffer == nullptr)
      continue;

    // Phase 2: no interleaving, no memcpy — VBOs already contain fresh data
    // written directly by the simulation system via mapped pointers.
    // We only need to link the attributes and issue the draw.
    mesh->bind();
    pool.gpu_buffer->linkToVao(1);
    mesh->unbind();

    Renderer::Renderer::drawInstanced(*mesh, pool.active_count, particleShader);
  }

  if (!query_in_flight_) {
    glEndQuery(GL_TIME_ELAPSED);
    query_in_flight_ = true;
  }
}

} // namespace ParticleGL::ECS::Systems

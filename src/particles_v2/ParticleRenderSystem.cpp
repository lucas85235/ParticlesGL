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

  // Retrieve async query result if one is in flight
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

  // Start new query if we aren't waiting on one
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

    // Phase 1 Optimization: Instead of an AoS struct representing the whole
    // buffer, we still have to interleave the SoA arrays into the VBO because
    // the current `InstanceBuffer` expects an AoS memory layout. In Phase 2
    // this will be replaced by a proper multi-VBO or compute-mapped buffer.
    // Notice we only interleave what is active.

    // We define a local AoS struct just for the upload payload to match the VBO
    struct UploadPayload {
      glm::vec3 pos;
      float scale;
      glm::vec4 col;
    };

    std::vector<UploadPayload> upload_buffer;
    upload_buffer.reserve(pool.active_count);

    for (uint32_t i = 0; i < pool.active_count; ++i) {
      upload_buffer.push_back({pool.position[i], pool.scale[i], pool.color[i]});
    }

    // Flush to GPU
    pool.gpu_buffer->updateData(upload_buffer.data(), pool.active_count);

    // Draw
    mesh->bind();
    pool.gpu_buffer->linkToVao(1);
    mesh->unbind();

    Renderer::Renderer::drawInstanced(*mesh, *pool.gpu_buffer, particleShader);
  }

  if (!query_in_flight_) {
    glEndQuery(GL_TIME_ELAPSED);
    query_in_flight_ = true;
  }
}

} // namespace ParticleGL::ECS::Systems

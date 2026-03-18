#include "ParticleSimulationSystem.hpp"
#include "../ecs/components/ParticleEmitter.hpp"
#include "../ecs/components/Transform.hpp"
#include "ParticlePoolComponent.hpp"
#include "core/Logger.hpp"
#include "renderer/GpuParticleBuffer.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <random>
#include <vector>

namespace ParticleGL::ECS::Systems {

ParticleSimulationSystem::ParticleSimulationSystem() = default;

void ParticleSimulationSystem::lazyInit() {
  if (initialized_)
    return;

  simulate_shader_ = Renderer::ComputeShader::loadFromFile(
      "assets/shaders/particle_simulate.comp");
  compact_shader_ = Renderer::ComputeShader::loadFromFile(
      "assets/shaders/particle_compact.comp");

  if (!simulate_shader_ || !compact_shader_) {
    PGL_ERROR("ParticleSimulationSystem: failed to load compute shaders.");
    return;
  }

  PGL_INFO("ParticleSimulationSystem: compute shaders loaded.");
  initialized_ = true;
}

void ParticleSimulationSystem::update(Registry &registry, float dt) {
  lazyInit();
  if (!initialized_)
    return;

  auto emitters = registry.getEntitiesWith<ECS::Components::ParticleEmitter>();

  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<float> rand_spread(-1.0f, 1.0f);

  for (Entity entity : emitters) {
    if (!registry.hasComponent<ECS::Components::Transform>(entity) ||
        !registry.hasComponent<Particles::ParticlePoolComponent>(entity)) {
      continue;
    }

    auto &emitter =
        registry.getComponent<ECS::Components::ParticleEmitter>(entity);
    auto &transform = registry.getComponent<ECS::Components::Transform>(entity);
    auto &pool =
        registry.getComponent<Particles::ParticlePoolComponent>(entity);

    if (!pool.gpu_buffer)
      continue;

    Renderer::GpuParticleBuffer &gpuBuf = *pool.gpu_buffer;

    // ── Spawn Phase ─────────────────────────────────────────────────────────
    // Build spawn batch on CPU, upload via glBufferSubData at the tail of
    // active.
    float toSpawnF = dt * emitter.emissionRate + pool.emission_accumulator;
    uint32_t toSpawn = static_cast<uint32_t>(toSpawnF);
    pool.emission_accumulator = toSpawnF - static_cast<float>(toSpawn);

    uint32_t available = pool.max_count > pool.active_count
                             ? pool.max_count - pool.active_count
                             : 0;
    toSpawn = std::min(toSpawn, available);

    if (toSpawn > 0) {
      std::vector<Renderer::GpuParticleBuffer::SpawnData> batch(toSpawn);

      const float speed = glm::length(emitter.initialVelocity);
      const glm::vec3 baseDir = speed > 0.001f
                                    ? glm::normalize(emitter.initialVelocity)
                                    : glm::vec3(0.0f, 1.0f, 0.0f);

      for (uint32_t i = 0; i < toSpawn; ++i) {
        glm::vec3 spray(rand_spread(gen), rand_spread(gen), rand_spread(gen));
        glm::vec3 dir = baseDir + spray * (emitter.spreadAngle / 90.0f);
        dir = glm::length(dir) > 0.001f ? glm::normalize(dir) : baseDir;

        batch[i].position = glm::vec4(transform.position, 0.0f);
        batch[i].velocity = glm::vec4(dir * speed, 0.0f);
        batch[i].life = 0.0f;
        batch[i].maxLife = emitter.particleLifetime;
        batch[i].color = emitter.startColor;
        batch[i].startColor = emitter.startColor;
        batch[i].endColor = emitter.endColor;
        batch[i].scale = 1.0f;
      }

      gpuBuf.uploadSpawnBatch(pool.active_count, batch.data(), toSpawn);
      pool.active_count += toSpawn;
      gpuBuf.setActiveCount(pool.active_count);
    }

    if (pool.active_count == 0)
      continue;

    // ── Dispatch particle_simulate.comp ─────────────────────────────────────
    gpuBuf.bindSsbos();

    simulate_shader_->bind();
    simulate_shader_->setFloat("u_dt", dt);
    simulate_shader_->setVec3("u_gravity", glm::vec3(0.0f, -2.0f, 0.0f));

    const uint32_t groups = (pool.active_count + 63) / 64;
    simulate_shader_->dispatch(groups);

    // Barrier: SSBOs and atomic counters must be coherent before compact
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
                    GL_ATOMIC_COUNTER_BARRIER_BIT);

    // ── Dispatch particle_compact.comp ───────────────────────────────────────
    compact_shader_->bind();
    compact_shader_->dispatch(1);

    // Barrier: draw indirect command and vertex attrib SSBOs must be ready
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
                    GL_ATOMIC_COUNTER_BARRIER_BIT | GL_COMMAND_BARRIER_BIT |
                    GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    compact_shader_->unbind();

    // Update CPU-side active_count for stall-free inspector polling
    // (async — the value may lag one frame, which is acceptable for UI)
    pool.active_count = gpuBuf.readActiveCount();
    emitter.activeParticles = pool.active_count;
  }
}

} // namespace ParticleGL::ECS::Systems

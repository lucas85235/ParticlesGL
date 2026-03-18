#include "ParticleSimulationSystem.hpp"
#include "../ecs/components/ParticleEmitter.hpp"
#include "../ecs/components/Transform.hpp"
#include "ParticlePoolComponent.hpp"

#include <random>

namespace ParticleGL::ECS::Systems {

void ParticleSimulationSystem::update(Registry &registry, float dt) {
  auto emitters = registry.getEntitiesWith<ECS::Components::ParticleEmitter>();

  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<float> rand_spread(-1.0f, 1.0f);

  for (Entity entity : emitters) {
    if (!registry.hasComponent<ECS::Components::Transform>(entity)) {
      continue;
    }

    auto &emitter =
        registry.getComponent<ECS::Components::ParticleEmitter>(entity);
    auto &transform = registry.getComponent<ECS::Components::Transform>(entity);

    if (!registry.hasComponent<Particles::ParticlePoolComponent>(entity)) {
      Particles::ParticlePoolComponent new_pool;
      new_pool.init(emitter.maxParticles);
      registry.addComponent<Particles::ParticlePoolComponent>(
          entity, std::move(new_pool));
    }

    auto &pool =
        registry.getComponent<Particles::ParticlePoolComponent>(entity);

    if (pool.max_count != emitter.maxParticles) {
      pool.init(emitter.maxParticles);
    }

    // Phase 2: orphan all 3 VBOs and map them for unsynchronized CPU write.
    // This eliminates the GPU stall that glBufferSubData caused.
    if (pool.gpu_buffer) {
      pool.gpu_buffer->beginWrite();
    }

    // ── Spawning Phase ──────────────────────────────────────────────────────
    float toSpawnFloat = dt * emitter.emissionRate + pool.emission_accumulator;
    uint32_t toSpawn = static_cast<uint32_t>(toSpawnFloat);
    pool.emission_accumulator = toSpawnFloat - static_cast<float>(toSpawn);

    for (uint32_t i = 0; i < toSpawn; ++i) {
      glm::vec3 spray(rand_spread(gen), rand_spread(gen), rand_spread(gen));
      float speed = glm::length(emitter.initialVelocity);
      glm::vec3 baseDir = speed > 0.001f
                              ? glm::normalize(emitter.initialVelocity)
                              : glm::vec3(0.0f, 1.0f, 0.0f);

      glm::vec3 dir = baseDir + spray * (emitter.spreadAngle / 90.0f);
      dir = glm::length(dir) > 0.001f ? glm::normalize(dir) : baseDir;

      // emit() writes directly into the mapped VBO pointers
      if (!pool.emit(transform.position, dir * speed, emitter.startColor, 1.0f,
                     emitter.particleLifetime)) {
        break;
      }
    }

    // ── Simulation Phase — SoA backward sweep ──────────────────────────────
    for (int i = static_cast<int>(pool.active_count) - 1; i >= 0; --i) {
      pool.life[i] += dt;

      if (pool.life[i] >= pool.maxLife[i]) {
        pool.kill(i);
        continue;
      }

      const float t = pool.life[i] / pool.maxLife[i];

      if (pool.gpu_buffer) {
        pool.gpu_buffer->positionPtr()[i] += pool.velocity[i] * dt;
        pool.gpu_buffer->colorPtr()[i] =
            glm::mix(emitter.startColor, emitter.endColor, t);
        pool.gpu_buffer->scalePtr()[i] = 1.0f * (1.0f - t);
      }
    }

    // Unmap VBOs before the draw call — GPU will read the freshly written data
    if (pool.gpu_buffer) {
      pool.gpu_buffer->endWrite();
      pool.gpu_buffer->setActiveCount(pool.active_count);
    }

    emitter.activeParticles = pool.active_count;
  }
}

} // namespace ParticleGL::ECS::Systems

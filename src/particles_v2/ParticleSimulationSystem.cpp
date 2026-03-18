#include "ParticleSimulationSystem.hpp"
#include "../ecs/components/ParticleEmitter.hpp"
#include "../ecs/components/Transform.hpp"
#include "ParticlePoolComponent.hpp"

#include <random>

namespace ParticleGL::ECS::Systems {

void ParticleSimulationSystem::update(Registry &registry, float dt) {
  // We need all entities that have Emitter AND Transform.
  // We will auto-attach the ParticlePoolComponent if missing.
  auto emitters = registry.getEntitiesWith<ECS::Components::ParticleEmitter>();

  // Setup basic random generation for spread
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

    // 1. Ensure Pool component exists and is sized correctly
    if (!registry.hasComponent<Particles::ParticlePoolComponent>(entity)) {
      Particles::ParticlePoolComponent new_pool;
      new_pool.init(emitter.maxParticles);
      registry.addComponent<Particles::ParticlePoolComponent>(
          entity, std::move(new_pool));
    }
    auto &pool =
        registry.getComponent<Particles::ParticlePoolComponent>(entity);

    // If maxParticles changed in inspector, we just force a re-init
    // (This clears the pool in the POC for simplicity, but prevents crashes)
    if (pool.max_count != emitter.maxParticles) {
      pool.init(emitter.maxParticles);
    }

    // 2. Spawning Phase
    float toSpawnFloat = dt * emitter.emissionRate + pool.emission_accumulator;
    uint32_t toSpawnInt = static_cast<uint32_t>(toSpawnFloat);
    pool.emission_accumulator = toSpawnFloat - static_cast<float>(toSpawnInt);

    for (uint32_t i = 0; i < toSpawnInt; ++i) {
      glm::vec3 spray(rand_spread(gen), rand_spread(gen), rand_spread(gen));
      float speed = glm::length(emitter.initialVelocity);
      glm::vec3 baseDir = speed > 0.001f
                              ? glm::normalize(emitter.initialVelocity)
                              : glm::vec3(0.0f, 1.0f, 0.0f);

      glm::vec3 dir = baseDir + spray * (emitter.spreadAngle / 90.0f);
      if (glm::length(dir) > 0.001f) {
        dir = glm::normalize(dir);
      } else {
        dir = baseDir;
      }

      glm::vec3 vel = dir * speed;

      // O(1) emit — will fail cleanly if full
      if (!pool.emit(transform.position, vel, emitter.startColor, 1.0f,
                     emitter.particleLifetime)) {
        break;
      }
    }

    // 3. Simulation Phase — strict SoA iteration
    // Iterate backwards so kill() swap-and-decrement is safe in O(1)
    uint32_t activeCount = pool.active_count;
    for (int i = static_cast<int>(activeCount) - 1; i >= 0; --i) {
      pool.life[i] += dt;

      if (pool.life[i] >= pool.maxLife[i]) {
        pool.kill(i);
        continue;
      }

      // Integrate Physics
      pool.position[i] += pool.velocity[i] * dt;

      // Interpolate Properties
      float lifePct = pool.life[i] / pool.maxLife[i];

      // Color mix
      pool.color[i] = glm::mix(emitter.startColor, emitter.endColor, lifePct);

      // Scale down
      pool.scale[i] = 1.0f * (1.0f - lifePct);
    }

    // Sync active particles to emitter so UI can display it
    emitter.activeParticles = pool.active_count;
  }
}

} // namespace ParticleGL::ECS::Systems

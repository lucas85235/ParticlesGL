#include "ParticleSystem.hpp"
#include "ecs/components/ParticleEmitter.hpp"
#include "ecs/components/Transform.hpp"
#include <random>

namespace ParticleGL::ECS::Systems {

void ParticleSystem::update(Registry &registry, float dt) {
  auto candidates = registry.getEntitiesWith<Components::ParticleEmitter>();

  // We don't have a native sparse-set view yet, so we iterate all entities
  // that have both Transform and ParticleEmitter.
  // In a real ECS we'd have a Registry::view<T, U...>()

  // Simplistic iteration for the POC: Assume active entities are 0 to
  // next_entity_id_ - 1 minus free list, but without Registry exposing active
  // entities, we will just iterate 0 to an arbitrary max or expose active
  // entities. Actually, getting all entities requires Registry to expose
  // them. Let's fix Registry first.

  // Setup basic random generation for spread
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<float> rand_spread(-1.0f, 1.0f);

  for (Entity entity : candidates) {
    if (!registry.hasComponent<Components::Transform>(entity)) {
      continue;
    }

    auto &transform = registry.getComponent<Components::Transform>(entity);
    auto &emitter = registry.getComponent<Components::ParticleEmitter>(entity);

    // Ensure pool exists and sync max count if changed
    if (pools_.find(entity) == pools_.end() ||
        pools_.at(entity).getMaxParticles() != emitter.maxParticles) {
      // This instantiates a new Pool (and VBO underneath). Emplacement uses the
      // int max_particles constructor.
      pools_.erase(entity);
      pools_.emplace(std::piecewise_construct, std::forward_as_tuple(entity),
                     std::forward_as_tuple(emitter.maxParticles));
      emission_accumulators_[entity] = 0.0f;
    }

    auto &pool = pools_.at(entity);

    // 1. Spawning Phase
    float particlesToSpawnFloat =
        dt * emitter.emissionRate + emission_accumulators_[entity];
    uint32_t particlesToSpawn = static_cast<uint32_t>(particlesToSpawnFloat);
    emission_accumulators_[entity] =
        particlesToSpawnFloat - static_cast<float>(particlesToSpawn);

    for (uint32_t i = 0; i < particlesToSpawn; ++i) {
      Particles::ParticleInstanceData pInst;
      pInst.position = transform.position;
      pInst.scale = 1.0f;
      pInst.color = glm::vec4(1.0f);

      Particles::ParticleSimData pSim;
      // Simple outward spread based on initial velocity direction + random
      // spray
      glm::vec3 randomVelocity(rand_spread(gen), rand_spread(gen),
                               rand_spread(gen));

      // Normalize and scale by emitter speed if you had one in ParticleEmitter,
      // using a hardcoded spray for now
      pSim.velocity = randomVelocity * 2.0f;
      pSim.life = 0.0f;
      pSim.maxLife = 2.0f; // Could be moved to ParticleEmitter config

      if (!pool.emit(pInst, pSim)) {
        break; // Pool is full
      }
    }

    // 2. Simulation Phase (Iterate backwards to allow Safe Deletion/Swapping in
    // O(1))
    uint32_t activeCount = pool.getActiveParticleCount();
    for (int i = static_cast<int>(activeCount) - 1; i >= 0; --i) {
      auto &sim = pool.getSimData(i);
      auto &inst = pool.getInstanceData(i);

      sim.life += dt;

      // Kill if dead
      if (sim.life >= sim.maxLife) {
        pool.kill(i);
        continue; // Element i is dead and swapped, the new element at i will be
                  // simulated next frame
      }

      // Integrate Physics
      inst.position += sim.velocity * dt;

      // Interpolate Properties over life
      float lifePct = sim.life / sim.maxLife;

      // Fade out
      inst.color.a = 1.0f - lifePct;

      // Scale down
      inst.scale = 1.0f * (1.0f - lifePct);
    }

    // Sync emitter status tracking (useful for UI)
    emitter.activeParticles = pool.getActiveParticleCount();
  }

  // Note: Calling flushToGPU isn't done here because the Update Loop shouldn't
  // make OpenGL calls The renderer loop handles flushing before
  // `glDrawElementsInstanced`.
}

} // namespace ParticleGL::ECS::Systems

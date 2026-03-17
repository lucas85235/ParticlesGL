#pragma once

#include "ecs/System.hpp"
#include "particles/ParticlePool.hpp"
#include <unordered_map>

namespace ParticleGL::ECS::Systems {

class ParticleSystem : public System {
public:
  ParticleSystem() = default;
  ~ParticleSystem() override = default;

  void update(Registry &registry, float dt) override;

  // Direct access to the internal pools for rendering
  std::unordered_map<Entity, Particles::ParticlePool> &getPools() {
    return pools_;
  }

private:
  // One memory pool per emitting entity
  std::unordered_map<Entity, Particles::ParticlePool> pools_;

  // Accumulator for fractional emissions
  std::unordered_map<Entity, float> emission_accumulators_;
};

} // namespace ParticleGL::ECS::Systems

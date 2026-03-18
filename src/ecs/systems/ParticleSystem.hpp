// DEPRECATED: use particles_v2/ParticleSimulationSystem.hpp instead.
// This file is kept only to compile existing tests without modification.
#pragma once

#include "ecs/System.hpp"
#include "particles/ParticlePool.hpp"
#include <unordered_map>

namespace ParticleGL::ECS::Systems {

class ParticleSystem_Deprecated : public System {
public:
  ParticleSystem_Deprecated() = default;
  ~ParticleSystem_Deprecated() override = default;

  void update(Registry &registry, float dt) override;

  std::unordered_map<Entity, Particles::ParticlePool> &getPools() {
    return pools_;
  }

private:
  std::unordered_map<Entity, Particles::ParticlePool> pools_;
  std::unordered_map<Entity, float> emission_accumulators_;
};

// Legacy alias so existing code (tests, Application) keeps compiling
using ParticleSystem = ParticleSystem_Deprecated;

} // namespace ParticleGL::ECS::Systems

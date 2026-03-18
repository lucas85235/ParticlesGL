#pragma once

#include "../ecs/System.hpp"

namespace ParticleGL::ECS::Systems {

class ParticleSimulationSystem : public System {
public:
  ParticleSimulationSystem() = default;
  ~ParticleSimulationSystem() override = default;

  void update(Registry &registry, float dt) override;
};

} // namespace ParticleGL::ECS::Systems

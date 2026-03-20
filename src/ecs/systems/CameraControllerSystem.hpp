#pragma once

#include "../Registry.hpp"
#include "../System.hpp"

namespace ParticleGL::ECS::Systems {

class CameraControllerSystem : public System {
public:
  void update(Registry &registry, float dt) override;
};

} // namespace ParticleGL::ECS::Systems

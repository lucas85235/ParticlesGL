#pragma once

#include "Registry.hpp"

namespace ParticleGL::ECS {

class System {
public:
  virtual ~System() = default;

  // Core game loop update hook.
  // dt = Delta time in seconds since last frame.
  virtual void update(Registry &registry, float dt) = 0;
};

} // namespace ParticleGL::ECS

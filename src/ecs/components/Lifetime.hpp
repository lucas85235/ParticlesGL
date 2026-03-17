#pragma once

namespace ParticleGL::ECS::Components {

struct Lifetime {
  float current{0.0f};
  float max{1.0f};
  bool active{true};
};

} // namespace ParticleGL::ECS::Components

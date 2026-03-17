#pragma once

#include <cstdint>

namespace ParticleGL::ECS::Components {

struct Renderable {
  uint32_t meshId{0};
  uint32_t shaderId{0};
};

} // namespace ParticleGL::ECS::Components

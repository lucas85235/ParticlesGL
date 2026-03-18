#pragma once

#include <glm/glm.hpp>
#include <string>

namespace ParticleGL::ECS::Components {

struct Renderable {
  std::string meshPath{""};
  std::string materialId{""};
};

} // namespace ParticleGL::ECS::Components

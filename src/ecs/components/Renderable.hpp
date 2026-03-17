#pragma once

#include <glm/glm.hpp>
#include <string>

namespace ParticleGL::ECS::Components {

struct Renderable {
  std::string meshPath{""};
  std::string materialId{""};
  glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f}; // Default white
};

} // namespace ParticleGL::ECS::Components

#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "Shader.hpp"

namespace ParticleGL::Renderer {

struct Material {
  std::string id;
  std::shared_ptr<Shader> shader;
  glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};

  Material() = default;
  Material(const std::string &id, std::shared_ptr<Shader> shader)
      : id(id), shader(shader) {}

  void bind() const {
    if (shader) {
      shader->bind();
      shader->setVec4("u_BaseColor", baseColor);
    }
  }

  void unbind() const {
    if (shader) {
      shader->unbind();
    }
  }
};

} // namespace ParticleGL::Renderer

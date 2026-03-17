#pragma once

#include "Camera.hpp"
#include "InstanceBuffer.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace ParticleGL::Renderer {

class Renderer {
public:
  static void init();
  static void shutdown();

  static void setViewport(uint32_t width, uint32_t height);
  static void setClearColor(const glm::vec4 &color);
  static void clear();

  static void beginScene(const Camera &camera);
  static void endScene();

  // Standard draw
  static void draw(const Mesh &mesh, const Shader &shader,
                   const glm::mat4 &transform);

  // Instanced draw for particles
  static void drawInstanced(const Mesh &mesh,
                            const InstanceBuffer &instanceBuffer,
                            const Shader &shader);

private:
  static const Camera *active_camera_;
};

} // namespace ParticleGL::Renderer

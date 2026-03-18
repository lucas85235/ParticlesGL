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

  // Instanced draw for particles — legacy path (uses InstanceBuffer active
  // count)
  static void drawInstanced(const Mesh &mesh,
                            const InstanceBuffer &instanceBuffer,
                            const Shader &shader);

  // Instanced draw for particles — Phase 2 path (explicit count, no
  // InstanceBuffer)
  static void drawInstanced(const Mesh &mesh, uint32_t instanceCount,
                            const Shader &shader);

  // GPU-indirect draw: sets u_ViewProjection then reads the draw command from
  // the currently bound GL_DRAW_INDIRECT_BUFFER. Binding the buffer is the
  // caller's responsibility.
  static void drawIndirect(const Mesh &mesh, const Shader &shader);

private:
  static const Camera *active_camera_;
};

} // namespace ParticleGL::Renderer

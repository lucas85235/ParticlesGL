#include "Renderer.hpp"
#include "core/Logger.hpp"
#include <glad/glad.h>

namespace ParticleGL::Renderer {

const Camera *Renderer::active_camera_ = nullptr;

void Renderer::init() {
  // Global OpenGL state configuration
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // Blend settings for particles
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::shutdown() {
  // Cleanup if needed
}

void Renderer::setViewport(uint32_t width, uint32_t height) {
  glViewport(0, 0, width, height);
}

void Renderer::setClearColor(const glm::vec4 &color) {
  glClearColor(color.r, color.g, color.b, color.a);
}

void Renderer::clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

void Renderer::beginScene(const Camera &camera) { active_camera_ = &camera; }

void Renderer::endScene() { active_camera_ = nullptr; }

void Renderer::draw(const Mesh &mesh, const Shader &shader,
                    const glm::mat4 &transform) {
  if (!active_camera_) {
    PGL_ERROR("Renderer::draw called outside of beginScene/endScene!");
    return;
  }

  shader.bind();
  shader.setMat4("u_ViewProjection", active_camera_->getViewProjectionMatrix());
  shader.setMat4("u_Transform", transform);

  mesh.bind();
  glDrawElements(GL_TRIANGLES, mesh.getIndexCount(), GL_UNSIGNED_INT, nullptr);
  mesh.unbind();
}

void Renderer::drawInstanced(const Mesh &mesh,
                             const InstanceBuffer &instanceBuffer,
                             const Shader &shader) {
  if (!active_camera_) {
    PGL_ERROR("Renderer::drawInstanced called outside of beginScene/endScene!");
    return;
  }

  uint32_t instanceCount = instanceBuffer.getActiveInstances();
  if (instanceCount == 0)
    return;

  shader.bind();
  shader.setMat4("u_ViewProjection", active_camera_->getViewProjectionMatrix());

  mesh.bind();
  glDrawElementsInstanced(GL_TRIANGLES, mesh.getIndexCount(), GL_UNSIGNED_INT,
                          nullptr, instanceCount);
  mesh.unbind();
}

void Renderer::drawInstanced(const Mesh &mesh, uint32_t instanceCount,
                             const Shader &shader) {
  if (!active_camera_) {
    PGL_ERROR("Renderer::drawInstanced called outside of beginScene/endScene!");
    return;
  }

  if (instanceCount == 0)
    return;

  shader.bind();
  shader.setMat4("u_ViewProjection", active_camera_->getViewProjectionMatrix());

  mesh.bind();
  glDrawElementsInstanced(GL_TRIANGLES, mesh.getIndexCount(), GL_UNSIGNED_INT,
                          nullptr, instanceCount);
  mesh.unbind();
}

void Renderer::drawIndirect(const Mesh &mesh, const Shader &shader) {
  if (!active_camera_) {
    PGL_ERROR("Renderer::drawIndirect called outside of beginScene/endScene!");
    return;
  }
  shader.bind();
  shader.setMat4("u_ViewProjection", active_camera_->getViewProjectionMatrix());
  mesh.bind();
  // GL_DRAW_INDIRECT_BUFFER must already be bound by the caller
  glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
  mesh.unbind();
}

void Renderer::drawMultiIndirect(const Mesh &mesh, const Shader &shader, uint32_t drawCount) {
  if (!active_camera_) {
    PGL_ERROR("Renderer::drawMultiIndirect called outside of beginScene/endScene!");
    return;
  }
  if (drawCount == 0) return;
  
  shader.bind();
  shader.setMat4("u_ViewProjection", active_camera_->getViewProjectionMatrix());
  mesh.bind();
  // GL_DRAW_INDIRECT_BUFFER must already be bound by the caller
  glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, drawCount, 0);
  mesh.unbind();
}

} // namespace ParticleGL::Renderer

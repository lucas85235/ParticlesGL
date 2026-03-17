#pragma once

#include <glm/glm.hpp>

namespace ParticleGL::Renderer {

class Camera {
public:
  Camera(uint32_t width, uint32_t height);
  ~Camera() = default;

  void setViewportSize(uint32_t width, uint32_t height);

  // View Matrix components
  const glm::vec3 &getPosition() const { return position_; }
  void setPosition(const glm::vec3 &position) {
    position_ = position;
    updateViewMatrix();
  }

  float getPitch() const { return pitch_; }
  float getYaw() const { return yaw_; }
  void setRotation(float pitch, float yaw);

  // Projection Matrix components
  float getFov() const { return fov_; }
  void setFov(float fov) {
    fov_ = fov;
    updateProjectionMatrix();
  }

  float getNearClip() const { return near_clip_; }
  float getFarClip() const { return far_clip_; }
  void setClips(float nearClip, float farClip) {
    near_clip_ = nearClip;
    far_clip_ = farClip;
    updateProjectionMatrix();
  }

  // Result Matrices
  const glm::mat4 &getViewMatrix() const { return view_matrix_; }
  const glm::mat4 &getProjectionMatrix() const { return projection_matrix_; }
  const glm::mat4 &getViewProjectionMatrix() const {
    return view_projection_matrix_;
  }

  // Helpers
  glm::vec3 getFront() const { return front_; }
  glm::vec3 getUp() const { return up_; }
  glm::vec3 getRight() const { return right_; }

private:
  void updateViewMatrix();
  void updateProjectionMatrix();
  void updateVectors();

  uint32_t viewport_width_{1280};
  uint32_t viewport_height_{720};

  glm::vec3 position_{0.0f, 0.0f, 5.0f};

  // Euler Angles
  float pitch_{0.0f};
  float yaw_{-90.0f};

  // Camera basis
  glm::vec3 front_{0.0f, 0.0f, -1.0f};
  glm::vec3 up_{0.0f, 1.0f, 0.0f};
  glm::vec3 right_{1.0f, 0.0f, 0.0f};
  glm::vec3 world_up_{0.0f, 1.0f, 0.0f};

  // Projection parameters
  float fov_{45.0f};
  float near_clip_{0.1f};
  float far_clip_{1000.0f};

  // Matrices cache
  glm::mat4 view_matrix_{1.0f};
  glm::mat4 projection_matrix_{1.0f};
  glm::mat4 view_projection_matrix_{1.0f};
};

} // namespace ParticleGL::Renderer

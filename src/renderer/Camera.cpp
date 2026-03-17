#include "Camera.hpp"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace ParticleGL::Renderer {

Camera::Camera(uint32_t width, uint32_t height) {
  setViewportSize(width, height);
  updateVectors();
}

void Camera::setViewportSize(uint32_t width, uint32_t height) {
  if (width > 0 && height > 0) {
    viewport_width_ = width;
    viewport_height_ = height;
    updateProjectionMatrix();
  }
}

void Camera::setRotation(float pitch, float yaw) {
  pitch_ = std::clamp(pitch, -89.0f, 89.0f);
  yaw_ = yaw;
  updateVectors();
}

void Camera::updateVectors() {
  glm::vec3 front;
  front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  front.y = sin(glm::radians(pitch_));
  front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  front_ = glm::normalize(front);

  right_ = glm::normalize(glm::cross(front_, world_up_));
  up_ = glm::normalize(glm::cross(right_, front_));

  updateViewMatrix();
}

void Camera::updateViewMatrix() {
  view_matrix_ = glm::lookAt(position_, position_ + front_, up_);
  view_projection_matrix_ = projection_matrix_ * view_matrix_;
}

void Camera::updateProjectionMatrix() {
  float aspect_ratio = static_cast<float>(viewport_width_) /
                       static_cast<float>(viewport_height_);
  projection_matrix_ =
      glm::perspective(glm::radians(fov_), aspect_ratio, near_clip_, far_clip_);
  view_projection_matrix_ = projection_matrix_ * view_matrix_;
}

} // namespace ParticleGL::Renderer

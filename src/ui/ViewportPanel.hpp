#pragma once

#include <functional>
#include <glm/glm.hpp>

namespace ParticleGL::UI {

class ViewportPanel {
public:
  ViewportPanel() = default;
  ~ViewportPanel() = default;

  using ResizeCallbackFn = std::function<void(uint32_t, uint32_t)>;

  void setResizeCallback(const ResizeCallbackFn &callback) {
    resize_callback_ = callback;
  }

  // Must be called inside the ImGui rendering phase
  void onImGuiRender(uint32_t textureID);

  // Call at the START of the frame (before bind/render) to apply any pending
  // resize without invalidating the FBO mid-frame.
  void applyPendingResize();

  glm::vec2 getViewportSize() const { return viewport_size_; }
  bool isFocused() const { return is_focused_; }
  bool isHovered() const { return is_hovered_; }

private:
  glm::vec2 viewport_size_{0.0f, 0.0f};
  glm::vec2 pending_size_{0.0f, 0.0f};
  bool has_pending_resize_{false};
  bool is_focused_{false};
  bool is_hovered_{false};
  ResizeCallbackFn resize_callback_;
};

} // namespace ParticleGL::UI

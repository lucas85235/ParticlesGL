#include "ViewportPanel.hpp"
#include <imgui.h>

namespace ParticleGL::UI {

void ViewportPanel::applyPendingResize() {
  if (!has_pending_resize_) {
    return;
  }
  has_pending_resize_ = false;
  viewport_size_ = pending_size_;
  if (resize_callback_) {
    resize_callback_(static_cast<uint32_t>(viewport_size_.x),
                     static_cast<uint32_t>(viewport_size_.y));
  }
}

void ViewportPanel::onImGuiRender(uint32_t textureID) {
  // Remove window padding for the viewport so the image fills the entire pane
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
  ImGui::SetNextWindowSize(ImVec2{800, 600}, ImGuiCond_FirstUseEver);
  ImGui::Begin("Viewport");

  is_focused_ = ImGui::IsWindowFocused();
  is_hovered_ = ImGui::IsWindowHovered();

  ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
  glm::vec2 currentSize = {viewportPanelSize.x, viewportPanelSize.y};

  // Only record the intent to resize; the actual resize is deferred to the
  // start of the next frame via applyPendingResize() to avoid invalidating the
  // FBO texture on the same frame it is consumed by ImGui::Image.
  if (viewport_size_ != currentSize && currentSize.x > 0.0f &&
      currentSize.y > 0.0f) {
    pending_size_ = currentSize;
    has_pending_resize_ = true;
  }

  // Draw the Framebuffer texture. ImGui expects a void pointer.
  // We flip the V coordinate since OpenGL textures are loaded bottom-to-top
  // natively.
  ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(textureID)),
               ImVec2{viewport_size_.x, viewport_size_.y}, ImVec2{0, 1},
               ImVec2{1, 0});

  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace ParticleGL::UI

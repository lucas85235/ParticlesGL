#pragma once

#include <cstdint>

namespace ParticleGL::Renderer {

struct FramebufferSpecification {
  uint32_t width, height;
  bool depth_only = false; // When true, creates a depth-only FBO (no color attachment)
};

class Framebuffer {
public:
  Framebuffer(const FramebufferSpecification &spec);
  ~Framebuffer();

  // Move semantics
  Framebuffer(const Framebuffer &) = delete;
  Framebuffer &operator=(const Framebuffer &) = delete;
  Framebuffer(Framebuffer &&other) noexcept;
  Framebuffer &operator=(Framebuffer &&other) noexcept;

  void bind();
  void unbind();

  void resize(uint32_t width, uint32_t height);

  uint32_t getColorAttachmentRendererID() const { return color_attachment_; }
  uint32_t getDepthAttachmentRendererID() const { return depth_attachment_; }
  const FramebufferSpecification &getSpecification() const { return spec_; }

private:
  void invalidate();

  uint32_t renderer_id_{0};
  uint32_t color_attachment_{0};
  uint32_t depth_attachment_{0};
  FramebufferSpecification spec_;
};

} // namespace ParticleGL::Renderer

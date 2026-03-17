#pragma once

#include <cstdint>

namespace ParticleGL::Renderer {

struct FramebufferSpecification {
  uint32_t width, height;
  // Options like swapChainTarget, samples, etc. could go here for a full engine
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
  const FramebufferSpecification &getSpecification() const { return spec_; }

private:
  void invalidate();

  uint32_t renderer_id_{0};
  uint32_t color_attachment_{0};
  uint32_t depth_attachment_{0};
  FramebufferSpecification spec_;
};

} // namespace ParticleGL::Renderer

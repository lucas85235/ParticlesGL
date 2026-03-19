#include "Framebuffer.hpp"
#include "../core/Logger.hpp"
#include <glad/glad.h>

namespace ParticleGL::Renderer {

static const uint32_t s_max_framebuffer_size = 8192;

Framebuffer::Framebuffer(const FramebufferSpecification &spec) : spec_(spec) {
  invalidate(); // Constructing invokes OpenGL creation immediately
}

Framebuffer::~Framebuffer() {
  if (renderer_id_ != 0) {
    glDeleteFramebuffers(1, &renderer_id_);
    glDeleteTextures(1, &color_attachment_);
    glDeleteTextures(1, &depth_attachment_);
  }
}

Framebuffer::Framebuffer(Framebuffer &&other) noexcept
    : renderer_id_(other.renderer_id_),
      color_attachment_(other.color_attachment_),
      depth_attachment_(other.depth_attachment_), spec_(other.spec_) {
  other.renderer_id_ = 0;
  other.color_attachment_ = 0;
  other.depth_attachment_ = 0;
}

Framebuffer &Framebuffer::operator=(Framebuffer &&other) noexcept {
  if (this != &other) {
    if (renderer_id_) {
      glDeleteFramebuffers(1, &renderer_id_);
      glDeleteTextures(1, &color_attachment_);
      glDeleteTextures(1, &depth_attachment_);
    }

    renderer_id_ = other.renderer_id_;
    color_attachment_ = other.color_attachment_;
    depth_attachment_ = other.depth_attachment_;
    spec_ = other.spec_;

    other.renderer_id_ = 0;
    other.color_attachment_ = 0;
    other.depth_attachment_ = 0;
  }
  return *this;
}

void Framebuffer::invalidate() {
  if (renderer_id_) {
    glDeleteFramebuffers(1, &renderer_id_);
    if (color_attachment_) glDeleteTextures(1, &color_attachment_);
    glDeleteTextures(1, &depth_attachment_);
    renderer_id_ = 0;
    color_attachment_ = 0;
    depth_attachment_ = 0;
  }

  glGenFramebuffers(1, &renderer_id_);
  glBindFramebuffer(GL_FRAMEBUFFER, renderer_id_);

  if (spec_.depth_only) {
    // Depth Pre-Pass: only depth texture, no color attachment.
    // Used to sample scene depth in compute shaders for particle collision.
    glGenTextures(1, &depth_attachment_);
    glBindTexture(GL_TEXTURE_2D, depth_attachment_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, spec_.width, spec_.height,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, depth_attachment_, 0);
    // Explicitly disable color reads/writes
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
  } else {
    // Standard FBO: color + depth/stencil
    glGenTextures(1, &color_attachment_);
    glBindTexture(GL_TEXTURE_2D, color_attachment_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, spec_.width, spec_.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           color_attachment_, 0);

    glGenTextures(1, &depth_attachment_);
    glBindTexture(GL_TEXTURE_2D, depth_attachment_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, spec_.width, spec_.height,
                 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                           GL_TEXTURE_2D, depth_attachment_, 0);
  }

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    PGL_ERROR("Framebuffer is incomplete!");
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::bind() {
  glBindFramebuffer(GL_FRAMEBUFFER, renderer_id_);
  glViewport(0, 0, spec_.width, spec_.height);
}

void Framebuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void Framebuffer::resize(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0 || width > s_max_framebuffer_size ||
      height > s_max_framebuffer_size) {
    PGL_ERROR("Attempted to resize framebuffer to " << width << ", " << height);
    return;
  }
  spec_.width = width;
  spec_.height = height;
  invalidate(); // Recreate FBO resources
}

} // namespace ParticleGL::Renderer

#include "InstanceBuffer.hpp"
#include <glad/glad.h>

namespace ParticleGL::Renderer {

InstanceBuffer::InstanceBuffer(uint32_t maxInstances,
                               const std::vector<uint32_t> &layout)
    : max_instances_(maxInstances), layout_(layout) {

  // Compute stride
  uint32_t stride = 0;
  for (uint32_t count : layout_) {
    stride += count;
  }
  stride_bytes_ = stride * sizeof(float);

#ifndef PGL_TEST_ENV
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  // Allocate mutable buffer space (usage: GL_DYNAMIC_DRAW)
  glBufferData(GL_ARRAY_BUFFER, max_instances_ * stride_bytes_, nullptr,
               GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

InstanceBuffer::~InstanceBuffer() {
#ifndef PGL_TEST_ENV
  if (vbo_ != 0) {
    glDeleteBuffers(1, &vbo_);
  }
#endif
}

InstanceBuffer::InstanceBuffer(InstanceBuffer &&other) noexcept
    : vbo_(other.vbo_), max_instances_(other.max_instances_),
      active_instances_(other.active_instances_),
      stride_bytes_(other.stride_bytes_), layout_(std::move(other.layout_)) {
  other.vbo_ = 0;
  other.max_instances_ = 0;
  other.active_instances_ = 0;
  other.stride_bytes_ = 0;
}

InstanceBuffer &InstanceBuffer::operator=(InstanceBuffer &&other) noexcept {
  if (this != &other) {
#ifndef PGL_TEST_ENV
    if (vbo_ != 0) {
      glDeleteBuffers(1, &vbo_);
    }
#endif

    vbo_ = other.vbo_;
    max_instances_ = other.max_instances_;
    active_instances_ = other.active_instances_;
    stride_bytes_ = other.stride_bytes_;
    layout_ = std::move(other.layout_);

    other.vbo_ = 0;
    other.max_instances_ = 0;
    other.active_instances_ = 0;
    other.stride_bytes_ = 0;
  }
  return *this;
}

void InstanceBuffer::linkToVao(uint32_t startingAttribIndex) const {
#ifndef PGL_TEST_ENV
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);

  uint32_t offset = 0;
  for (uint32_t i = 0; i < layout_.size(); ++i) {
    uint32_t attribLocation = startingAttribIndex + i;
    glEnableVertexAttribArray(attribLocation);
    glVertexAttribPointer(attribLocation, layout_[i], GL_FLOAT, GL_FALSE,
                          stride_bytes_, reinterpret_cast<void *>(offset));

    // This is what makes it an instanced attribute
    glVertexAttribDivisor(attribLocation, 1);

    offset += layout_[i] * sizeof(float);
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

void InstanceBuffer::updateData(const void *data, uint32_t instanceCount) {
  active_instances_ =
      (instanceCount > max_instances_) ? max_instances_ : instanceCount;

#ifndef PGL_TEST_ENV
  if (active_instances_ > 0) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    // Use glBufferSubData to selectively update just the active block
    glBufferSubData(GL_ARRAY_BUFFER, 0, active_instances_ * stride_bytes_,
                    data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
#endif
}

} // namespace ParticleGL::Renderer

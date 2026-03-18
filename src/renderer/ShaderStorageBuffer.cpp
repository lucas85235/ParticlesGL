#include "ShaderStorageBuffer.hpp"
#include "core/Logger.hpp"

#include <glad/glad.h>

namespace ParticleGL::Renderer {

ShaderStorageBuffer::ShaderStorageBuffer(size_t sizeBytes, const void *data)
    : size_bytes_(sizeBytes) {
  glGenBuffers(1, &id_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(sizeBytes),
               data, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  PGL_INFO("SSBO created (id=" << id_ << ", " << sizeBytes << " bytes)");
}

ShaderStorageBuffer::~ShaderStorageBuffer() {
  if (id_ != 0) {
    glDeleteBuffers(1, &id_);
  }
}

ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer &&other) noexcept
    : id_(other.id_), size_bytes_(other.size_bytes_) {
  other.id_ = 0;
  other.size_bytes_ = 0;
}

ShaderStorageBuffer &
ShaderStorageBuffer::operator=(ShaderStorageBuffer &&other) noexcept {
  if (this != &other) {
    if (id_ != 0) {
      glDeleteBuffers(1, &id_);
    }
    id_ = other.id_;
    size_bytes_ = other.size_bytes_;
    other.id_ = 0;
    other.size_bytes_ = 0;
  }
  return *this;
}

void ShaderStorageBuffer::bindBase(uint32_t bindingPoint) const {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, id_);
}

void ShaderStorageBuffer::upload(const void *data, size_t sizeBytes,
                                 size_t offsetBytes) {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, static_cast<GLintptr>(offsetBytes),
                  static_cast<GLsizeiptr>(sizeBytes), data);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::download(void *outData, size_t sizeBytes,
                                   size_t offsetBytes) const {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER,
                     static_cast<GLintptr>(offsetBytes),
                     static_cast<GLsizeiptr>(sizeBytes), outData);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

} // namespace ParticleGL::Renderer

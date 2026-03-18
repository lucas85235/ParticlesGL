#pragma once

#include <cstddef>
#include <cstdint>

namespace ParticleGL::Renderer {

// Thin RAII wrapper for an OpenGL Shader Storage Buffer Object (SSBO).
// Binds to a user-supplied binding point matching the GLSL `layout(std430,
// binding = N)`.
class ShaderStorageBuffer {
public:
  // Allocates a GPU buffer of |sizeBytes| bytes.
  // If |data| is non-null the buffer is pre-filled with its contents.
  ShaderStorageBuffer(size_t sizeBytes, const void *data = nullptr);
  ~ShaderStorageBuffer();

  ShaderStorageBuffer(const ShaderStorageBuffer &) = delete;
  ShaderStorageBuffer &operator=(const ShaderStorageBuffer &) = delete;

  ShaderStorageBuffer(ShaderStorageBuffer &&other) noexcept;
  ShaderStorageBuffer &operator=(ShaderStorageBuffer &&other) noexcept;

  // Binds the buffer to the specified GLSL binding point.
  void bindBase(uint32_t bindingPoint) const;

  // Uploads |sizeBytes| bytes from |data| starting at |offsetBytes| in the GPU
  // buffer.
  void upload(const void *data, size_t sizeBytes, size_t offsetBytes = 0);

  // Downloads |sizeBytes| bytes from the GPU buffer starting at |offsetBytes|
  // into |outData|.
  void download(void *outData, size_t sizeBytes, size_t offsetBytes = 0) const;

  uint32_t getId() const { return id_; }
  size_t getSizeBytes() const { return size_bytes_; }

private:
  uint32_t id_{0};
  size_t size_bytes_{0};
};

} // namespace ParticleGL::Renderer

#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace ParticleGL::Renderer {

// Double-buffered GL 4.3 particle instance buffer.
//
// Uses 3 separate VBOs (position / scale / color) matching the SoA CPU layout
// so no interleaving is needed on the CPU side.
//
// Upload strategy: buffer orphaning + GL_MAP_UNSYNCHRONIZED_BIT.
// Each frame the buffer is re-specified as empty (orphan) before mapping,
// which lets the driver avoid waiting for the previous draw to complete.
// This is the GL 4.3-compatible equivalent of GL_MAP_PERSISTENT_BIT.
//
// Usage per frame:
//   beginWrite()   — orphan + map all VBOs, returns writable pointers
//   fill pointers via positionPtr() / scalePtr() / colorPtr()
//   endWrite()     — unmap all VBOs
//   draw call
class MappedInstanceBuffer {
public:
  explicit MappedInstanceBuffer(uint32_t maxInstances);
  ~MappedInstanceBuffer();

  MappedInstanceBuffer(const MappedInstanceBuffer &) = delete;
  MappedInstanceBuffer &operator=(const MappedInstanceBuffer &) = delete;

  // Map all VBOs for writing. Must be followed by endWrite() before draw.
  void beginWrite();
  void endWrite();

  // Valid only between beginWrite() / endWrite()
  glm::vec3 *positionPtr() { return pos_ptr_; }
  float *scalePtr() { return scale_ptr_; }
  glm::vec4 *colorPtr() { return color_ptr_; }

  // Call while the target Mesh VAO is bound.
  // Links location 1 = position (vec3), 2 = scale (float), 3 = color (vec4).
  void linkToVao(uint32_t startAttrib) const;

  void setActiveCount(uint32_t count);
  uint32_t getActiveInstances() const { return active_instances_; }
  uint32_t getMaxInstances() const { return max_instances_; }

private:
  uint32_t max_instances_ = 0;
  uint32_t active_instances_ = 0;

  uint32_t vbo_position_ = 0;
  uint32_t vbo_scale_ = 0;
  uint32_t vbo_color_ = 0;

  glm::vec3 *pos_ptr_ = nullptr;
  float *scale_ptr_ = nullptr;
  glm::vec4 *color_ptr_ = nullptr;
};

// Legacy alias used by Application and ParticlePoolComponent
using PersistentInstanceBuffer = MappedInstanceBuffer;

} // namespace ParticleGL::Renderer

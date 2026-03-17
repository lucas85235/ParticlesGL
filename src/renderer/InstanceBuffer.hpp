#pragma once

#include <cstdint>
#include <vector>

namespace ParticleGL::Renderer {

// Defines per-instance attributes layout (e.g. vec3 pos, vec4 color)
class InstanceBuffer {
public:
  // Layout defines the number of floats per attribute.
  // E.g., {3, 4} for a vec3 position and vec4 color
  InstanceBuffer(uint32_t maxInstances, const std::vector<uint32_t> &layout);
  ~InstanceBuffer();

  // Prevent copying
  InstanceBuffer(const InstanceBuffer &) = delete;
  InstanceBuffer &operator=(const InstanceBuffer &) = delete;

  // Allow moving
  InstanceBuffer(InstanceBuffer &&other) noexcept;
  InstanceBuffer &operator=(InstanceBuffer &&other) noexcept;

  // Call this while a Mesh VAO is bound to link the instance attributes to it
  // startingAttribIndex should be the first index available after the mesh
  // attributes
  void linkToVao(uint32_t startingAttribIndex) const;

  // Set dynamic block of data (avoids reallocation)
  void updateData(const void *data, uint32_t instanceCount);

  uint32_t getMaxInstances() const { return max_instances_; }
  uint32_t getActiveInstances() const { return active_instances_; }

private:
  uint32_t vbo_{0};
  uint32_t max_instances_{0};
  uint32_t active_instances_{0};
  uint32_t stride_bytes_{0};
  std::vector<uint32_t> layout_;
};

} // namespace ParticleGL::Renderer

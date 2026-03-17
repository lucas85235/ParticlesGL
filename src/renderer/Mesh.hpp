#pragma once

#include <cstdint>
#include <vector>

namespace ParticleGL::Renderer {

class Mesh {
public:
  // Takes interleaved vertex data (e.g., pos, normal, uv)
  // Layout defines the number of floats for each attribute.
  // E.g. {3, 2} means position is vec3 and uv is vec2.
  Mesh(const std::vector<float> &vertices, const std::vector<uint32_t> &indices,
       const std::vector<uint32_t> &layout);
  ~Mesh();

  // Prevent copying
  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  // Allow moving
  Mesh(Mesh &&other) noexcept;
  Mesh &operator=(Mesh &&other) noexcept;

  void bind() const;
  void unbind() const;

  uint32_t getIndexCount() const { return index_count_; }
  uint32_t getVaoId() const { return vao_; }

private:
  uint32_t vao_{0};
  uint32_t vbo_{0};
  uint32_t ebo_{0};
  uint32_t index_count_{0};
};

} // namespace ParticleGL::Renderer

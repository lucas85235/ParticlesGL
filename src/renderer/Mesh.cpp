#include "Mesh.hpp"
#include <glad/glad.h>
#include <numeric>

namespace ParticleGL::Renderer {

Mesh::Mesh(const std::vector<float> &vertices,
           const std::vector<uint32_t> &indices,
           const std::vector<uint32_t> &layout)
    : index_count_(static_cast<uint32_t>(indices.size())) {

  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);
  glGenBuffers(1, &ebo_);

  glBindVertexArray(vao_);

  // VBO setup
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
               vertices.data(), GL_STATIC_DRAW);

  // EBO setup
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
               indices.data(), GL_STATIC_DRAW);

  // Compute stride
  uint32_t stride = 0;
  for (uint32_t count : layout) {
    stride += count;
  }
  stride *= sizeof(float);

  // Setup attributes
  uint32_t offset = 0;
  for (uint32_t i = 0; i < layout.size(); ++i) {
    glEnableVertexAttribArray(i);
    glVertexAttribPointer(i, layout[i], GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offset));
    offset += layout[i] * sizeof(float);
  }

  // Unbind VAO (safe to unbind VBO as well, but NOT EBO while VAO is bound)
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  // ebo is safely captured in the vao
}

Mesh::~Mesh() {
  if (vao_ != 0) {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ebo_);
  }
}

Mesh::Mesh(Mesh &&other) noexcept
    : vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_),
      index_count_(other.index_count_) {
  other.vao_ = 0;
  other.vbo_ = 0;
  other.ebo_ = 0;
  other.index_count_ = 0;
}

Mesh &Mesh::operator=(Mesh &&other) noexcept {
  if (this != &other) {
    if (vao_ != 0) {
      glDeleteVertexArrays(1, &vao_);
      glDeleteBuffers(1, &vbo_);
      glDeleteBuffers(1, &ebo_);
    }

    vao_ = other.vao_;
    vbo_ = other.vbo_;
    ebo_ = other.ebo_;
    index_count_ = other.index_count_;

    other.vao_ = 0;
    other.vbo_ = 0;
    other.ebo_ = 0;
    other.index_count_ = 0;
  }
  return *this;
}

void Mesh::bind() const { glBindVertexArray(vao_); }

void Mesh::unbind() const { glBindVertexArray(0); }

} // namespace ParticleGL::Renderer

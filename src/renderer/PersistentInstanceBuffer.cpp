#include "PersistentInstanceBuffer.hpp"
#include "core/Logger.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace ParticleGL::Renderer {

// Helper: create one STREAM_DRAW VBO of `count` elements of type T.
template <typename T> static void createStreamVbo(GLuint &vbo, uint32_t count) {
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  // Pre-allocate with GL_STREAM_DRAW — tells the driver we will re-specify
  // this data every frame (orphaning pattern).
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(count * sizeof(T)),
               nullptr, GL_STREAM_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Helper: orphan + map a VBO for unsynchronized writing.
// Orphaning (re-specifying size with nullptr) lets the driver allocate new
// backing storage without waiting for the GPU to finish using the old one.
template <typename T> static T *orphanAndMap(GLuint vbo, uint32_t count) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  // Orphan: discard the previous allocation
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(count * sizeof(T)),
               nullptr, GL_STREAM_DRAW);
  // Map unsynchronized: CPU can immediately write without waiting for GPU
  void *ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0,
                               static_cast<GLsizeiptr>(count * sizeof(T)),
                               GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return static_cast<T *>(ptr);
}

MappedInstanceBuffer::MappedInstanceBuffer(uint32_t maxInstances)
    : max_instances_(maxInstances) {

#ifndef PGL_TEST_ENV
  createStreamVbo<glm::vec3>(vbo_position_, maxInstances);
  createStreamVbo<float>(vbo_scale_, maxInstances);
  createStreamVbo<glm::vec4>(vbo_color_, maxInstances);

  PGL_INFO("MappedInstanceBuffer allocated: "
           << maxInstances
           << " slots (orphaning + unsynchronized map strategy)");
#endif
}

MappedInstanceBuffer::~MappedInstanceBuffer() {
#ifndef PGL_TEST_ENV
  // Make sure we're not mid-map
  if (pos_ptr_) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position_);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    pos_ptr_ = nullptr;
  }
  if (scale_ptr_) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_scale_);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    scale_ptr_ = nullptr;
  }
  if (color_ptr_) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_color_);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    color_ptr_ = nullptr;
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  if (vbo_position_)
    glDeleteBuffers(1, &vbo_position_);
  if (vbo_scale_)
    glDeleteBuffers(1, &vbo_scale_);
  if (vbo_color_)
    glDeleteBuffers(1, &vbo_color_);
#endif
}

void MappedInstanceBuffer::beginWrite() {
#ifndef PGL_TEST_ENV
  // Orphan all 3 VBOs and map them for unsynchronized write.
  pos_ptr_ = orphanAndMap<glm::vec3>(vbo_position_, max_instances_);
  scale_ptr_ = orphanAndMap<float>(vbo_scale_, max_instances_);
  color_ptr_ = orphanAndMap<glm::vec4>(vbo_color_, max_instances_);

  if (!pos_ptr_ || !scale_ptr_ || !color_ptr_) {
    PGL_WARN(
        "MappedInstanceBuffer::beginWrite: glMapBufferRange returned null.");
  }
#endif
}

void MappedInstanceBuffer::endWrite() {
#ifndef PGL_TEST_ENV
  if (pos_ptr_) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position_);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    pos_ptr_ = nullptr;
  }
  if (scale_ptr_) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_scale_);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    scale_ptr_ = nullptr;
  }
  if (color_ptr_) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_color_);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    color_ptr_ = nullptr;
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

void MappedInstanceBuffer::linkToVao(uint32_t startAttrib) const {
#ifndef PGL_TEST_ENV
  // position — vec3, one per instance
  glBindBuffer(GL_ARRAY_BUFFER, vbo_position_);
  glEnableVertexAttribArray(startAttrib);
  glVertexAttribPointer(startAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                        nullptr);
  glVertexAttribDivisor(startAttrib, 1);

  // scale — float, one per instance
  glBindBuffer(GL_ARRAY_BUFFER, vbo_scale_);
  glEnableVertexAttribArray(startAttrib + 1);
  glVertexAttribPointer(startAttrib + 1, 1, GL_FLOAT, GL_FALSE, sizeof(float),
                        nullptr);
  glVertexAttribDivisor(startAttrib + 1, 1);

  // color — vec4, one per instance
  glBindBuffer(GL_ARRAY_BUFFER, vbo_color_);
  glEnableVertexAttribArray(startAttrib + 2);
  glVertexAttribPointer(startAttrib + 2, 4, GL_FLOAT, GL_FALSE,
                        sizeof(glm::vec4), nullptr);
  glVertexAttribDivisor(startAttrib + 2, 1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

void MappedInstanceBuffer::setActiveCount(uint32_t count) {
  active_instances_ = (count > max_instances_) ? max_instances_ : count;
}

} // namespace ParticleGL::Renderer

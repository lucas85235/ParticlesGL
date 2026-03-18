#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace ParticleGL::Renderer {

// Wraps an OpenGL Compute Shader program (GL_COMPUTE_SHADER).
// Uniforms are set the same way as a regular Shader.
// Dispatch is performed via dispatch(groupsX, groupsY, groupsZ).
// Barriers are the caller's responsibility via barrier().
class ComputeShader {
public:
  static std::shared_ptr<ComputeShader> loadFromFile(const std::string &path);

  explicit ComputeShader(const std::string &source);
  ~ComputeShader();

  ComputeShader(const ComputeShader &) = delete;
  ComputeShader &operator=(const ComputeShader &) = delete;

  ComputeShader(ComputeShader &&other) noexcept;
  ComputeShader &operator=(ComputeShader &&other) noexcept;

  void bind() const;
  void unbind() const;

  // Issues a compute dispatch. Group counts must satisfy the global limit.
  void dispatch(uint32_t groupsX, uint32_t groupsY = 1,
                uint32_t groupsZ = 1) const;

  // Inserts a GL_BUFFER_UPDATE_BARRIER_BIT barrier so subsequent CPU reads
  // (glGetBufferSubData) see the compute shader's writes to SSBOs.
  static void barrier();

  // Uniform setters (same interface as Shader)
  void setInt(const std::string &name, int value) const;
  void setUInt(const std::string &name, uint32_t value) const;
  void setFloat(const std::string &name, float value) const;
  void setVec2(const std::string &name, const glm::vec2 &value) const;
  void setVec3(const std::string &name, const glm::vec3 &value) const;
  void setVec4(const std::string &name, const glm::vec4 &value) const;
  void setMat4(const std::string &name, const glm::mat4 &value) const;

  uint32_t getId() const { return id_; }

private:
  uint32_t id_{0};
  mutable std::unordered_map<std::string, int> uniform_location_cache_;

  int getUniformLocation(const std::string &name) const;
  void checkCompileErrors(uint32_t shader, const std::string &type);
};

} // namespace ParticleGL::Renderer

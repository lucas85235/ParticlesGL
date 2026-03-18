#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

#include <memory>

namespace ParticleGL::Renderer {

class Shader {
public:
  static std::shared_ptr<Shader> loadFromFile(const std::string &vertexPath,
                                              const std::string &fragmentPath);

  Shader(const std::string &vertexSource, const std::string &fragmentSource);
  ~Shader();

  // Prevent copying
  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;

  // Allow moving
  Shader(Shader &&other) noexcept;
  Shader &operator=(Shader &&other) noexcept;

  void bind() const;
  void unbind() const;

  // Uniform setters
  void setInt(const std::string &name, int value) const;
  void setFloat(const std::string &name, float value) const;
  void setVec2(const std::string &name, const glm::vec2 &value) const;
  void setVec3(const std::string &name, const glm::vec3 &value) const;
  void setVec4(const std::string &name, const glm::vec4 &value) const;
  void setMat4(const std::string &name, const glm::mat4 &value) const;

  // Get raw OpenGL ID
  uint32_t getId() const { return id_; }

private:
  uint32_t id_{0};
  mutable std::unordered_map<std::string, int> uniform_location_cache_;

  int getUniformLocation(const std::string &name) const;
  uint32_t compileShader(uint32_t type, const std::string &source);
  void checkCompileErrors(uint32_t shader, const std::string &type);
};

} // namespace ParticleGL::Renderer

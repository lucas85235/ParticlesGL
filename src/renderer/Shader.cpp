#include "Shader.hpp"
#include "core/Logger.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace ParticleGL::Renderer {

Shader::Shader(const std::string &vertexSource,
               const std::string &fragmentSource) {
  uint32_t vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
  uint32_t fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

  id_ = glCreateProgram();
  glAttachShader(id_, vertexShader);
  glAttachShader(id_, fragmentShader);
  glLinkProgram(id_);
  checkCompileErrors(id_, "PROGRAM");

  // Shaders are linked into the program and can now be deleted
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
}

Shader::~Shader() {
  if (id_ != 0) {
    glDeleteProgram(id_);
  }
}

Shader::Shader(Shader &&other) noexcept
    : id_(other.id_),
      uniform_location_cache_(std::move(other.uniform_location_cache_)) {
  other.id_ = 0;
}

Shader &Shader::operator=(Shader &&other) noexcept {
  if (this != &other) {
    if (id_ != 0) {
      glDeleteProgram(id_);
    }
    id_ = other.id_;
    uniform_location_cache_ = std::move(other.uniform_location_cache_);
    other.id_ = 0;
  }
  return *this;
}

void Shader::bind() const { glUseProgram(id_); }

void Shader::unbind() const { glUseProgram(0); }

void Shader::setInt(const std::string &name, int value) const {
  glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string &name, float value) const {
  glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string &name, const glm::vec2 &value) const {
  glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value) const {
  glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string &name, const glm::vec4 &value) const {
  glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string &name, const glm::mat4 &value) const {
  glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE,
                     glm::value_ptr(value));
}

int Shader::getUniformLocation(const std::string &name) const {
  if (uniform_location_cache_.find(name) != uniform_location_cache_.end()) {
    return uniform_location_cache_[name];
  }

  int location = glGetUniformLocation(id_, name.c_str());
  if (location == -1) {
    PGL_ERROR("Warning: uniform '" << name << "' doesn't exist!");
  }

  uniform_location_cache_[name] = location;
  return location;
}

uint32_t Shader::compileShader(uint32_t type, const std::string &source) {
  uint32_t shader = glCreateShader(type);
  const char *src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  std::string typeStr = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
  checkCompileErrors(shader, typeStr);

  return shader;
}

void Shader::checkCompileErrors(uint32_t shader, const std::string &type) {
  int success;
  char infoLog[1024];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
      PGL_ERROR(
          "ERROR::SHADER_COMPILATION_ERROR of type: "
          << type << "\n"
          << infoLog
          << "\n -- --------------------------------------------------- -- ");
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
      PGL_ERROR(
          "ERROR::PROGRAM_LINKING_ERROR of type: "
          << type << "\n"
          << infoLog
          << "\n -- --------------------------------------------------- -- ");
    }
  }
}

} // namespace ParticleGL::Renderer

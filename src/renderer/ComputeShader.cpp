#include "ComputeShader.hpp"
#include "core/Logger.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>

namespace ParticleGL::Renderer {

std::shared_ptr<ComputeShader>
ComputeShader::loadFromFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    PGL_ERROR("Failed to open compute shader file: " << path);
    return nullptr;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();
  if (source.empty()) {
    PGL_ERROR("Compute shader file is empty: " << path);
    return nullptr;
  }
  PGL_INFO("Loaded compute shader: " << path);
  return std::make_shared<ComputeShader>(source);
}

ComputeShader::ComputeShader(const std::string &source) {
  uint32_t cs = glCreateShader(GL_COMPUTE_SHADER);
  const char *src = source.c_str();
  glShaderSource(cs, 1, &src, nullptr);
  glCompileShader(cs);
  checkCompileErrors(cs, "COMPUTE");

  id_ = glCreateProgram();
  glAttachShader(id_, cs);
  glLinkProgram(id_);
  checkCompileErrors(id_, "PROGRAM");

  glDeleteShader(cs);
  PGL_INFO("Compute shader program linked (id=" << id_ << ")");
}

ComputeShader::~ComputeShader() {
  if (id_ != 0) {
    glDeleteProgram(id_);
  }
}

ComputeShader::ComputeShader(ComputeShader &&other) noexcept
    : id_(other.id_),
      uniform_location_cache_(std::move(other.uniform_location_cache_)) {
  other.id_ = 0;
}

ComputeShader &ComputeShader::operator=(ComputeShader &&other) noexcept {
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

void ComputeShader::bind() const { glUseProgram(id_); }

void ComputeShader::unbind() const { glUseProgram(0); }

void ComputeShader::dispatch(uint32_t groupsX, uint32_t groupsY,
                             uint32_t groupsZ) const {
  glDispatchCompute(groupsX, groupsY, groupsZ);
}

void ComputeShader::barrier() {
  // GL_BUFFER_UPDATE_BARRIER_BIT ensures compute shader writes are coherent
  // before subsequent CPU reads via glGetBufferSubData.
  // GL_SHADER_STORAGE_BARRIER_BIT only covers shader-to-shader coherency.
  glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

void ComputeShader::setInt(const std::string &name, int value) const {
  glUniform1i(getUniformLocation(name), value);
}

void ComputeShader::setUInt(const std::string &name, uint32_t value) const {
  // Use glUniform1ui, NOT glUniform1i, for GLSL `uint` uniforms.
  // glUniform1i on a uint uniform generates GL_INVALID_OPERATION silently,
  // leaving the uniform at its default value (0), which breaks range checks.
  glUniform1ui(getUniformLocation(name), value);
}

void ComputeShader::setFloat(const std::string &name, float value) const {
  glUniform1f(getUniformLocation(name), value);
}

void ComputeShader::setVec2(const std::string &name,
                            const glm::vec2 &value) const {
  glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void ComputeShader::setVec3(const std::string &name,
                            const glm::vec3 &value) const {
  glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void ComputeShader::setVec4(const std::string &name,
                            const glm::vec4 &value) const {
  glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void ComputeShader::setMat4(const std::string &name,
                            const glm::mat4 &value) const {
  glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE,
                     glm::value_ptr(value));
}

int ComputeShader::getUniformLocation(const std::string &name) const {
  auto it = uniform_location_cache_.find(name);
  if (it != uniform_location_cache_.end()) {
    return it->second;
  }
  int loc = glGetUniformLocation(id_, name.c_str());
  if (loc == -1) {
    PGL_ERROR("Compute shader uniform '" << name << "' not found");
  }
  uniform_location_cache_[name] = loc;
  return loc;
}

void ComputeShader::checkCompileErrors(uint32_t shader,
                                       const std::string &type) {
  int success;
  char infoLog[1024];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
      PGL_ERROR("Compute shader compile error (" << type << "):\n" << infoLog);
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
      PGL_ERROR("Compute shader link error (" << type << "):\n" << infoLog);
    }
  }
}

} // namespace ParticleGL::Renderer

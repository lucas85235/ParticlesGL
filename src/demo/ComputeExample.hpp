#pragma once

#include "renderer/ComputeShader.hpp"
#include "renderer/ShaderStorageBuffer.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace ParticleGL::Demo {

// A simple self-contained compute shader demonstration.
// Each tick:
//   1. Dispatches "color_wave.comp" on the GPU which writes an animated HSV
//      color wave into an SSBO containing N vec4 elements.
//   2. Reads back a single sample element (index 0) to CPU so the result can
//      be inspected or displayed in the UI without a full readback.
class ComputeExample {
public:
  static constexpr uint32_t kElementCount = 1024;

  ComputeExample() = default;
  ~ComputeExample() = default;

  ComputeExample(const ComputeExample &) = delete;
  ComputeExample &operator=(const ComputeExample &) = delete;

  // Loads shaders and allocates the SSBO. Must be called after an OpenGL
  // context is present.
  bool init(const std::string &computeShaderPath);

  // Dispatches the compute shader, issues a barrier, and reads sample[0] back.
  void tick(float time);

  // Returns the last readback of element[0] from the SSBO.
  glm::vec4 getSampleColor() const { return sample_color_; }

  bool isReady() const { return ready_; }

private:
  struct GpuParticle {
    glm::vec4 color{1.0f};
  };

  std::shared_ptr<Renderer::ComputeShader> shader_;
  std::unique_ptr<Renderer::ShaderStorageBuffer> ssbo_;
  glm::vec4 sample_color_{0.0f};
  bool ready_{false};
};

} // namespace ParticleGL::Demo

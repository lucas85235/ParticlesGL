#include "ComputeExample.hpp"
#include "core/Logger.hpp"

#include <cstring>

namespace ParticleGL::Demo {

bool ComputeExample::init(const std::string &computeShaderPath) {
  shader_ = Renderer::ComputeShader::loadFromFile(computeShaderPath);
  if (!shader_) {
    PGL_ERROR("ComputeExample: failed to load compute shader");
    return false;
  }

  // Allocate SSBO with zeroed initial data
  std::vector<GpuParticle> initial(kElementCount);
  ssbo_ = std::make_unique<Renderer::ShaderStorageBuffer>(
      initial.size() * sizeof(GpuParticle), initial.data());

  ready_ = true;
  PGL_INFO("ComputeExample initialised (" << kElementCount << " elements)");
  return true;
}

void ComputeExample::tick(float time) {
  if (!ready_) {
    return;
  }

  // 1. Bind the SSBO to binding point 0 (matches the compute shader layout)
  ssbo_->bindBase(0);

  // 2. Dispatch compute — one thread per element, in groups of 64
  shader_->bind();
  shader_->setFloat("u_Time", time);
  shader_->setUInt("u_Count", kElementCount);

  const uint32_t groups = (kElementCount + 63) / 64; // ceil(N / local_size_x)
  shader_->dispatch(groups);

  // 3. Memory barrier — ensure the GPU writes are visible before readback
  Renderer::ComputeShader::barrier();

  shader_->unbind();

  // 4. Read element[0] back to CPU for UI display
  GpuParticle sample;
  ssbo_->download(&sample, sizeof(GpuParticle), 0);
  sample_color_ = sample.color;
}

} // namespace ParticleGL::Demo

#pragma once

#include "renderer/ComputeShader.hpp"
#include "renderer/GpuParticleBuffer.hpp"
#include "ecs/Registry.hpp"
#include "ecs/components/ParticleEmitter.hpp"
#include "particles_v2/ParticlePoolComponent.hpp"

#include <glm/glm.hpp>
#include <memory>

namespace ParticleGL::ECS::Systems {

// Orchestrates 4 GPU compute passes to sort particle indices back-to-front
// per-emitter before rendering with Alpha blending.
//
// Pass order per emitter per frame (4 radix passes covering 32 bits):
//   1. radix_sort_init    — build distance sort keys + identity index buffer
//   2. radix_sort_histogram — build 256-bucket histogram for this 8-bit pass
//   3. radix_sort_scan    — exclusive prefix sum over the 256 histogram buckets
//   4. radix_sort_scatter — scatter indices to their sorted positions (ping-pong)
//
// Only dispatched for emitters that have blendMode == ParticleBlendMode::Alpha.
class RadixSortSystem {
public:
  RadixSortSystem() = default;

  void setGpuBuffer(Renderer::GpuParticleBuffer *buf) { gpu_buffer_ = buf; }
  void setCameraPosition(const glm::vec3 &pos) { camera_pos_ = pos; }

  // Sort all Alpha-blend emitters. Must be called before ParticleRenderSystem::render().
  void sort(Registry &registry);

private:
  void lazyInit();

  bool initialized_ = false;
  Renderer::GpuParticleBuffer *gpu_buffer_ = nullptr;
  glm::vec3 camera_pos_{0.0f};

  std::shared_ptr<Renderer::ComputeShader> init_shader_;
  std::shared_ptr<Renderer::ComputeShader> histogram_shader_;
  std::shared_ptr<Renderer::ComputeShader> scan_shader_;
  std::shared_ptr<Renderer::ComputeShader> scatter_shader_;
};

} // namespace ParticleGL::ECS::Systems

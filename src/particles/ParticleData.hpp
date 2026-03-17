#pragma once

#include <glm/glm.hpp>

namespace ParticleGL::Particles {

// POD struct for data sent to the GPU via Instance Buffer
struct ParticleInstanceData {
  glm::vec3 position{0.0f};
  float scale{1.0f};
  glm::vec4 color{1.0f};
};

// POD struct for simulation state running only on CPU
struct ParticleSimData {
  glm::vec3 velocity{0.0f};
  float life{0.0f};    // current age
  float maxLife{1.0f}; // total lifespan
};

} // namespace ParticleGL::Particles

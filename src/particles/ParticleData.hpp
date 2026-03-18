// DEPRECATED: use particles_v2/ParticlePoolComponent.hpp instead.
// This file is kept only to compile existing tests without modification.
#pragma once

#include <glm/glm.hpp>

namespace ParticleGL::Particles {

// POD struct for data sent to the GPU via Instance Buffer
struct ParticleInstanceData_Deprecated {
  glm::vec3 position{0.0f};
  float scale{1.0f};
  glm::vec4 color{1.0f};
};

// POD struct for simulation state running only on CPU
struct ParticleSimData_Deprecated {
  glm::vec3 velocity{0.0f};
  float life{0.0f};    // current age
  float maxLife{1.0f}; // total lifespan
};

// Legacy type aliases — still resolve for old includes
using ParticleInstanceData = ParticleInstanceData_Deprecated;
using ParticleSimData = ParticleSimData_Deprecated;

} // namespace ParticleGL::Particles

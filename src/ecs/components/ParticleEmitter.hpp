#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace ParticleGL::ECS::Components {

struct ParticleEmitter {
  float emissionRate{10.0f}; // Particles per second
  float timeSinceLastEmission{0.0f};

  glm::vec3 initialVelocity{0.0f, 1.0f, 0.0f};
  float spreadAngle{15.0f}; // In degrees

  glm::vec4 startColor{1.0f, 1.0f, 1.0f, 1.0f};
  glm::vec4 endColor{1.0f, 1.0f, 1.0f, 0.0f};

  float particleLifetime{2.0f};

  uint32_t maxParticles{1000};
  uint32_t activeParticles{0};
};

} // namespace ParticleGL::ECS::Components

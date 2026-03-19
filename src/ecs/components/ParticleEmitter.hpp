#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace ParticleGL::ECS::Components {

enum class ParticleBlendMode : uint8_t {
  Additive = 0,  // GL_SRC_ALPHA, GL_ONE  — order-independent, best for fire/plasma
  Alpha    = 1,  // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA — correct for smoke/cloud (requires sorting)
};

struct ParticleEmitter {
  float emissionRate{10.0f};
  float timeSinceLastEmission{0.0f};

  glm::vec3 initialVelocity{0.0f, 1.0f, 0.0f};
  float spreadAngle{15.0f};

  glm::vec4 startColor{1.0f, 1.0f, 1.0f, 1.0f};
  glm::vec4 endColor{1.0f, 1.0f, 1.0f, 0.0f};

  float particleLifetime{2.0f};

  uint32_t maxParticles{1000};
  uint32_t activeParticles{0};

  // Phase 5: Physics & Rendering
  bool collisionEnabled{true}; // Toggle depth and floor collisions
  float bounciness{0.5f};      // [0, 1] — energy retained after floor bounce (0 = no bounce, 1 = elastic)
  float friction{0.85f};       // [0, 1] — XZ velocity damping on floor impact
  float turbulence{0.0f};      // Curl noise field strength; 0 = purely kinematic
  float floorHeight{0.0f};     // World-space Y of the rigid floor plane

  ParticleBlendMode blendMode{ParticleBlendMode::Additive};
};

} // namespace ParticleGL::ECS::Components

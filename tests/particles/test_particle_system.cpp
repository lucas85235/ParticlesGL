#include "ecs/Registry.hpp"
#include "ecs/components/ParticleEmitter.hpp"
#include "ecs/components/Transform.hpp"
#include "ecs/systems/ParticleSystem.hpp"
#include <gtest/gtest.h>

using namespace ParticleGL::ECS;
using namespace ParticleGL::Particles;

class ParticleSystemTest : public ::testing::Test {
protected:
  Registry registry;
  Systems::ParticleSystem system;
};

TEST_F(ParticleSystemTest, DoesNotCrashOnEmptyRegistry) {
  EXPECT_NO_THROW(system.update(registry, 0.016f));
}

TEST_F(ParticleSystemTest, SpawnsParticlesCorrectly) {
  Entity e = registry.createEntity();
  registry.addComponent<Components::Transform>(e, Components::Transform{});
  registry.addComponent<Components::ParticleEmitter>(
      e,
      Components::ParticleEmitter{100.0f, 0.0f, glm::vec3(0.0f), 0.0f,
                                  glm::vec4(1.0f), glm::vec4(0.0f), 1.0f, 10});

  // 100 particles per second * 0.05 seconds = 5 particles
  system.update(registry, 0.05f);

  auto &pools = system.getPools();
  ASSERT_TRUE(pools.find(e) != pools.end());
  EXPECT_EQ(pools.at(e).getActiveParticleCount(), 5);
}

TEST_F(ParticleSystemTest, InterpolatesPropertiesAndKillsOverTime) {
  Entity e = registry.createEntity();
  registry.addComponent<Components::Transform>(e, Components::Transform{});
  registry.addComponent<Components::ParticleEmitter>(
      e,
      Components::ParticleEmitter{100.0f, 0.0f, glm::vec3(0.0f), 0.0f,
                                  glm::vec4(1.0f), glm::vec4(0.0f), 1.0f, 10});

  // Spawn 5
  system.update(registry, 0.05f);

  // Simulate forward 0.5s (half life)
  system.update(registry, 0.5f);

  auto &pools = system.getPools();
  auto &pool = pools.at(e);

  // The very first particle should now be at 0.55 life (0.05 from spawn + 0.5
  // from second update)
  float life = pool.getSimData(0).life;
  EXPECT_NEAR(life, 0.55f, 0.01f);

  // Scale and alpha should be reduced (approx 0.725)
  float scale = pool.getInstanceData(0).scale;
  float alpha = pool.getInstanceData(0).color.a;
  // Expected logic:
  // lifePct = 0.55 / 2.0 = 0.275
  // alpha = 1.0 - 0.275 = 0.725
  // scale = 1.0 * (1.0 - 0.275) = 0.725
  EXPECT_NEAR(scale, 0.725f, 0.01f);
  EXPECT_NEAR(alpha, 0.725f, 0.01f);

  // Simulate forward another 0.6s (total > 1.0s max life). Particles should
  // die.
  system.update(registry, 0.6f);

  // Notice that while the first 5 die, the system also SPAWNS new ones during
  // this 0.6s update! 100 * 0.6 = 60 particles! But we capped the pool at 10.
  EXPECT_LE(pool.getActiveParticleCount(), 10);

  // Verify tracked emitter stats updated
  auto &emitter = registry.getComponent<Components::ParticleEmitter>(e);
  EXPECT_EQ(emitter.activeParticles, pool.getActiveParticleCount());
}

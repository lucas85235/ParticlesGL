#include "ecs/Registry.hpp"
#include "ecs/System.hpp"
#include <gtest/gtest.h>

using namespace ParticleGL::ECS;

struct HealthComponent {
  int hp;
};

class MockDamageSystem : public System {
public:
  void update(Registry &registry, float dt) override {
    (void)dt; // Unused
    // We know what components we need, ideally we would iterate through a view
    // But for mock testing, we will just manually fetch for Entity 1
    if (registry.hasComponent<HealthComponent>(1)) {
      auto &health = registry.getComponent<HealthComponent>(1);
      health.hp -= 10;
    }
  }
};

TEST(SystemTest, UpdateInterface) {
  Registry registry;
  Entity e = registry.createEntity();  // Should be 0
  Entity e2 = registry.createEntity(); // Should be 1

  registry.addComponent<HealthComponent>(e2, {100});

  MockDamageSystem damageSystem;
  damageSystem.update(registry, 0.1f);

  EXPECT_EQ(registry.getComponent<HealthComponent>(e2).hp, 90);
}

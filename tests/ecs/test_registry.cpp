#include "ecs/Registry.hpp"
#include <gtest/gtest.h>

using namespace ParticleGL::ECS;

struct Position {
  float x, y;
};
struct Velocity {
  float dx, dy;
};

class RegistryTest : public ::testing::Test {
protected:
  Registry registry;
};

TEST_F(RegistryTest, CreateEntity) {
  Entity e1 = registry.createEntity();
  Entity e2 = registry.createEntity();
  EXPECT_NE(e1, e2);
  EXPECT_EQ(e1, 0);
  EXPECT_EQ(e2, 1);
}

TEST_F(RegistryTest, DestroyEntityAndRecycleID) {
  Entity e1 = registry.createEntity();
  Entity e2 = registry.createEntity();

  registry.destroyEntity(e1);

  // The next created entity should recycle e1's ID.
  Entity e3 = registry.createEntity();
  EXPECT_EQ(e3, e1);
}

TEST_F(RegistryTest, AddAndGetComponent) {
  Entity e = registry.createEntity();

  registry.addComponent<Position>(e, {10.0f, 20.0f});

  EXPECT_TRUE(registry.hasComponent<Position>(e));

  Position &pos = registry.getComponent<Position>(e);
  EXPECT_FLOAT_EQ(pos.x, 10.0f);
  EXPECT_FLOAT_EQ(pos.y, 20.0f);
}

TEST_F(RegistryTest, RemoveComponent) {
  Entity e = registry.createEntity();
  registry.addComponent<Position>(e, {1.0f, 1.0f});

  registry.removeComponent<Position>(e);
  EXPECT_FALSE(registry.hasComponent<Position>(e));
}

TEST_F(RegistryTest, DestroyEntityRemovesComponents) {
  Entity e = registry.createEntity();
  registry.addComponent<Position>(e, {1.0f, 1.0f});
  registry.addComponent<Velocity>(e, {0.5f, 0.5f});

  registry.destroyEntity(e);

  // When the ID is recycled, it shouldn't have the old components
  Entity eNew = registry.createEntity();
  EXPECT_EQ(eNew, e); // Make sure it recycled
  EXPECT_FALSE(registry.hasComponent<Position>(eNew));
  EXPECT_FALSE(registry.hasComponent<Velocity>(eNew));
}

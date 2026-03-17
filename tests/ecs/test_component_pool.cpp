#include "ecs/ComponentPool.hpp"
#include "ecs/Entity.hpp"
#include <gtest/gtest.h>

using namespace ParticleGL::ECS;

struct TestComponent {
  int value = 0;
  float flt = 0.0f;
};

TEST(ComponentPoolTest, InsertAndRetrieve) {
  ComponentPool<TestComponent> pool;
  Entity ent1 = 1;

  TestComponent &comp = pool.insert(ent1, {42, 3.14f});
  EXPECT_EQ(comp.value, 42);

  TestComponent &comp2 = pool.get(ent1);
  EXPECT_EQ(comp2.value, 42);
  EXPECT_FLOAT_EQ(comp2.flt, 3.14f);
  EXPECT_TRUE(pool.has(ent1));
  EXPECT_EQ(pool.size(), 1);
}

TEST(ComponentPoolTest, RemoveMaintainsContiguity) {
  ComponentPool<TestComponent> pool;
  Entity ent1 = 1;
  Entity ent2 = 2;
  Entity ent3 = 3;

  pool.insert(ent1, {10});
  pool.insert(ent2, {20});
  pool.insert(ent3, {30});

  EXPECT_EQ(pool.size(), 3);

  // Remove middle entity
  pool.remove(ent2);

  EXPECT_EQ(pool.size(), 2);
  EXPECT_FALSE(pool.has(ent2));
  EXPECT_TRUE(pool.has(ent1));
  EXPECT_TRUE(pool.has(ent3));

  // Entity 3 should now be at the index previously held by Entity 2
  TestComponent &c3 = pool.get(ent3);
  EXPECT_EQ(c3.value, 30);

  auto &data = pool.getData();
  // Data elements remaining should be 10, and 30
  EXPECT_EQ(data[0].value, 10);
  EXPECT_EQ(data[1].value, 30);
}

TEST(ComponentPoolTest, RemoveLastElement) {
  ComponentPool<TestComponent> pool;
  Entity ent1 = 1;
  Entity ent2 = 2;

  pool.insert(ent1, {10});
  pool.insert(ent2, {20});

  pool.remove(ent2);

  EXPECT_EQ(pool.size(), 1);
  EXPECT_FALSE(pool.has(ent2));
  EXPECT_TRUE(pool.has(ent1));

  EXPECT_EQ(pool.getData()[0].value, 10);
}

TEST(ComponentPoolTest, ClearPool) {
  ComponentPool<TestComponent> pool;
  Entity ent1 = 1;
  Entity ent2 = 2;

  pool.insert(ent1, {10});
  pool.insert(ent2, {20});

  pool.clear();

  EXPECT_EQ(pool.size(), 0);
  EXPECT_FALSE(pool.has(ent1));
  EXPECT_FALSE(pool.has(ent2));
}

TEST(ComponentPoolTest, PolymorphicRemove) {
  ComponentPool<TestComponent> pool;
  Entity ent1 = 1;

  pool.insert(ent1, {10});

  IComponentPool *ipool = &pool;
  ipool->removeEntity(ent1);

  EXPECT_EQ(pool.size(), 0);
  EXPECT_FALSE(pool.has(ent1));
}

#include "ecs/Registry.hpp"
#include "ecs/components/ParticleEmitter.hpp"
#include "ecs/components/Transform.hpp"
#include "serialization/SceneSerializer.hpp"
#include <gtest/gtest.h>

using namespace ParticleGL;
using namespace ParticleGL::ECS;

TEST(SceneSerializerTest, SerializeAndDeserialize) {
  Registry registry;

  Entity e1 = registry.createEntity();
  Components::Transform t1;
  t1.position = {1.0f, 2.0f, 3.0f};
  registry.addComponent(e1, t1);

  Entity e2 = registry.createEntity();
  Components::ParticleEmitter pe1;
  pe1.emissionRate = 42.0f;
  registry.addComponent(e2, pe1);

  Serialization::SceneSerializer serializer(&registry);
  serializer.serialize("test_scene.json");

  Registry new_registry;
  Serialization::SceneSerializer deserializer(&new_registry);
  bool result = deserializer.deserialize("test_scene.json");

  EXPECT_TRUE(result);
  // Entity 0 was the camera in actual app, but here we just test registry logic
  EXPECT_EQ(new_registry.getEntities().size(), 2);

  auto transforms = new_registry.getEntitiesWith<Components::Transform>();
  EXPECT_EQ(transforms.size(), 1);
  EXPECT_FLOAT_EQ(
      new_registry.getComponent<Components::Transform>(transforms[0])
          .position.x,
      1.0f);

  auto emitters = new_registry.getEntitiesWith<Components::ParticleEmitter>();
  EXPECT_EQ(emitters.size(), 1);
  EXPECT_FLOAT_EQ(
      new_registry.getComponent<Components::ParticleEmitter>(emitters[0])
          .emissionRate,
      42.0f);
}

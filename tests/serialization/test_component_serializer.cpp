#include "serialization/ComponentSerializer.hpp"
#include <gtest/gtest.h>

using namespace ParticleGL::ECS::Components;
using nlohmann::json;

TEST(ComponentSerializerTest, TransformSerialization) {
  Transform t1;
  t1.position = {1.0f, 2.0f, 3.0f};
  t1.rotation = glm::quat(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
  t1.scale = {0.5f, 0.5f, 0.5f};

  json j = t1;
  Transform t2 = j.get<Transform>();

  EXPECT_FLOAT_EQ(t1.position.x, t2.position.x);
  EXPECT_FLOAT_EQ(t1.position.y, t2.position.y);
  EXPECT_FLOAT_EQ(t1.position.z, t2.position.z);

  EXPECT_FLOAT_EQ(t1.rotation.x, t2.rotation.x);
  EXPECT_FLOAT_EQ(t1.rotation.y, t2.rotation.y);
  EXPECT_FLOAT_EQ(t1.rotation.z, t2.rotation.z);
  EXPECT_FLOAT_EQ(t1.rotation.w, t2.rotation.w);

  EXPECT_FLOAT_EQ(t1.scale.x, t2.scale.x);
  EXPECT_FLOAT_EQ(t1.scale.y, t2.scale.y);
  EXPECT_FLOAT_EQ(t1.scale.z, t2.scale.z);
}

TEST(ComponentSerializerTest, ParticleEmitterSerialization) {
  ParticleEmitter pe1;
  pe1.emissionRate = 50.0f;
  pe1.timeSinceLastEmission = 1.0f;
  pe1.initialVelocity = {0.0f, 5.0f, 0.0f};
  pe1.spreadAngle = 45.0f;
  pe1.startColor = {1.0f, 0.0f, 0.0f, 1.0f};
  pe1.endColor = {0.0f, 0.0f, 1.0f, 0.0f};
  pe1.particleLifetime = 3.5f;
  pe1.maxParticles = 500;
  pe1.activeParticles = 12;

  json j = pe1;
  ParticleEmitter pe2 = j.get<ParticleEmitter>();

  EXPECT_FLOAT_EQ(pe1.emissionRate, pe2.emissionRate);
  EXPECT_FLOAT_EQ(pe1.timeSinceLastEmission, pe2.timeSinceLastEmission);

  EXPECT_FLOAT_EQ(pe1.initialVelocity.x, pe2.initialVelocity.x);
  EXPECT_FLOAT_EQ(pe1.initialVelocity.y, pe2.initialVelocity.y);
  EXPECT_FLOAT_EQ(pe1.initialVelocity.z, pe2.initialVelocity.z);

  EXPECT_FLOAT_EQ(pe1.spreadAngle, pe2.spreadAngle);

  EXPECT_FLOAT_EQ(pe1.startColor.x, pe2.startColor.x);
  EXPECT_FLOAT_EQ(pe1.endColor.y, pe2.endColor.y);

  EXPECT_FLOAT_EQ(pe1.particleLifetime, pe2.particleLifetime);
  EXPECT_EQ(pe1.maxParticles, pe2.maxParticles);
  EXPECT_EQ(pe1.activeParticles, pe2.activeParticles);
}

TEST(ComponentSerializerTest, LifetimeSerialization) {
  Lifetime l1;
  l1.current = 1.5f;
  l1.max = 3.0f;
  l1.active = false;

  json j = l1;
  Lifetime l2 = j.get<Lifetime>();

  EXPECT_FLOAT_EQ(l1.current, l2.current);
  EXPECT_FLOAT_EQ(l1.max, l2.max);
  EXPECT_EQ(l1.active, l2.active);
}

#include "ecs/components/Lifetime.hpp"
#include "ecs/components/ParticleEmitter.hpp"
#include "ecs/components/Renderable.hpp"
#include "ecs/components/Transform.hpp"
#include <gtest/gtest.h>

#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/matrix_decompose.hpp>

using namespace ParticleGL::ECS::Components;

TEST(TransformComponentTest, DefaultValues) {
  Transform t;
  EXPECT_FLOAT_EQ(t.position.x, 0.0f);
  EXPECT_FLOAT_EQ(t.position.y, 0.0f);
  EXPECT_FLOAT_EQ(t.position.z, 0.0f);

  EXPECT_FLOAT_EQ(t.scale.x, 1.0f);
  EXPECT_FLOAT_EQ(t.scale.y, 1.0f);
  EXPECT_FLOAT_EQ(t.scale.z, 1.0f);
}

TEST(TransformComponentTest, MatrixGeneration) {
  Transform t;
  t.position = glm::vec3(1.0f, 2.0f, 3.0f);
  t.scale = glm::vec3(2.0f, 2.0f, 2.0f);

  glm::mat4 mat = t.matrix();

  glm::vec3 scale;
  glm::quat rotation;
  glm::vec3 translation;
  glm::vec3 skew;
  glm::vec4 perspective;
  glm::decompose(mat, scale, rotation, translation, skew, perspective);

  EXPECT_FLOAT_EQ(translation.x, 1.0f);
  EXPECT_FLOAT_EQ(translation.y, 2.0f);
  EXPECT_FLOAT_EQ(translation.z, 3.0f);

  EXPECT_FLOAT_EQ(scale.x, 2.0f);
  EXPECT_FLOAT_EQ(scale.y, 2.0f);
  EXPECT_FLOAT_EQ(scale.z, 2.0f);
}

TEST(ParticleEmitterComponentTest, DefaultValues) {
  ParticleEmitter pe;
  EXPECT_FLOAT_EQ(pe.emissionRate, 10.0f);
  EXPECT_GT(pe.maxParticles, 0);
  EXPECT_EQ(pe.activeParticles, 0);
}

TEST(RenderableComponentTest, DefaultValues) {
  Renderable r;
  EXPECT_EQ(r.meshId, 0);
  EXPECT_EQ(r.shaderId, 0);
}

TEST(LifetimeComponentTest, DefaultValues) {
  Lifetime l;
  EXPECT_FLOAT_EQ(l.current, 0.0f);
  EXPECT_FLOAT_EQ(l.max, 1.0f);
  EXPECT_TRUE(l.active);
}

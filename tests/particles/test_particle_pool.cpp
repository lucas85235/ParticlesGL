#include "particles/ParticlePool.hpp"
#include <gtest/gtest.h>

using namespace ParticleGL::Particles;

class ParticlePoolTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Suppress OpenGL errors for testing if no context is available
    // Usually, InstanceBuffer creation requires a GL context.
    // For these unit tests, we'll assume a dummy context was created or the
    // InstanceBuffer constructor handles missing GL context gracefully.
  }
};

TEST_F(ParticlePoolTest, InitializationAndLimits) {
  ParticlePool pool(100);
  EXPECT_EQ(pool.getMaxParticles(), 100);
  EXPECT_EQ(pool.getActiveParticleCount(), 0);
}

TEST_F(ParticlePoolTest, EmissionBehavior) {
  ParticlePool pool(2);

  ParticleInstanceData iData;
  ParticleSimData sData;

  EXPECT_TRUE(pool.emit(iData, sData));
  EXPECT_EQ(pool.getActiveParticleCount(), 1);

  EXPECT_TRUE(pool.emit(iData, sData));
  EXPECT_EQ(pool.getActiveParticleCount(), 2);

  // Should fail as pool is full
  EXPECT_FALSE(pool.emit(iData, sData));
  EXPECT_EQ(pool.getActiveParticleCount(), 2);
}

TEST_F(ParticlePoolTest, KillMaintainsContiguity) {
  ParticlePool pool(5);

  ParticleInstanceData iData;
  ParticleSimData sData;

  for (int i = 0; i < 5; ++i) {
    sData.life = static_cast<float>(i); // distinguish them
    pool.emit(iData, sData);
  }

  EXPECT_EQ(pool.getActiveParticleCount(), 5);

  // Kill index 1 (the one with life = 1.0)
  pool.kill(1);
  EXPECT_EQ(pool.getActiveParticleCount(), 4);

  // Because it swaps with the last element (which had life = 4.0), index 1
  // should now be the old last element
  EXPECT_FLOAT_EQ(pool.getSimData(1).life, 4.0f);
}

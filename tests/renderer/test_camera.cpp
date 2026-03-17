#include "renderer/Camera.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <gtest/gtest.h>

using namespace ParticleGL::Renderer;

class CameraTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Run before each test
  }

  // Helper to check vectors
  bool Vec3Equal(const glm::vec3 &a, const glm::vec3 &b,
                 float epsilon = 1e-4f) {
    return glm::all(glm::epsilonEqual(a, b, epsilon));
  }
};

TEST_F(CameraTest, DefinesDefaultProperties) {
  Camera cam(800, 600);

  EXPECT_TRUE(Vec3Equal(cam.getPosition(), glm::vec3(0.0f, 0.0f, 5.0f)));
  EXPECT_FLOAT_EQ(cam.getPitch(), 0.0f);
  EXPECT_FLOAT_EQ(cam.getYaw(), -90.0f); // Default is looking along -z
  EXPECT_FLOAT_EQ(cam.getFov(), 45.0f);
}

TEST_F(CameraTest, UpdatesPosition) {
  Camera cam(800, 600);

  cam.setPosition(glm::vec3(1.0f, 2.0f, 3.0f));
  EXPECT_TRUE(Vec3Equal(cam.getPosition(), glm::vec3(1.0f, 2.0f, 3.0f)));

  // Ensure moving updates the view matrix without changing the front vector
  // direction
  EXPECT_TRUE(Vec3Equal(cam.getFront(), glm::vec3(0.0f, 0.0f, -1.0f)));
}

TEST_F(CameraTest, UpdatesRotationVectorsCorrectly) {
  Camera cam(800, 600);

  // Look right
  cam.setRotation(0.0f, 0.0f);
  EXPECT_TRUE(Vec3Equal(cam.getFront(), glm::vec3(1.0f, 0.0f, 0.0f)));

  // Look up
  cam.setRotation(89.0f, -90.0f);
  // Pitch is constrained, evaluating mathematically
  EXPECT_TRUE(Vec3Equal(cam.getFront(), glm::vec3(0.0f, 1.0f, 0.0f), 0.02f));
}

TEST_F(CameraTest, ProjectionMatrixAspectCorrectness) {
  Camera cam(800, 600); // 4:3
  glm::mat4 proj43 = cam.getProjectionMatrix();

  cam.setViewportSize(1920, 1080); // 16:9
  glm::mat4 proj169 = cam.getProjectionMatrix();

  // Matrices should be different because of different aspect ratios
  EXPECT_NE(proj43[0][0], proj169[0][0]);
  EXPECT_FLOAT_EQ(proj43[1][1],
                  proj169[1][1]); // y scale should be consistent given same FOV
}

TEST_F(CameraTest, ClampsPitchRotation) {
  Camera cam(800, 600);

  cam.setRotation(150.0f, -90.0f);
  EXPECT_FLOAT_EQ(cam.getPitch(), 89.0f);

  cam.setRotation(-100.0f, -90.0f);
  EXPECT_FLOAT_EQ(cam.getPitch(), -89.0f);
}

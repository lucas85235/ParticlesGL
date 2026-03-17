#include "Application.hpp"
#include "core/Logger.hpp"
#include "ecs/Registry.hpp"
#include "ecs/components/Lifetime.hpp"
#include "ecs/components/ParticleEmitter.hpp"
#include "ecs/components/Renderable.hpp"
#include "ecs/components/Transform.hpp"
#include "ecs/systems/ParticleSystem.hpp"
#include "renderer/Camera.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/Shader.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#include "../ui/InspectorPanel.hpp"
#include "../ui/ScenePanel.hpp"
#include "../ui/StatsPanel.hpp"

#include <glad/glad.h>

#include <GLFW/glfw3.h>

namespace ParticleGL {

Application *Application::s_instance_ = nullptr;

Application::Application() {
  if (s_instance_) {
    PGL_ERROR("Application already exists!");
    return;
  }
  s_instance_ = this;

  window_ = std::make_unique<Window>();
  ui_layer_ = std::make_unique<UI::UILayer>();
}

Application::~Application() { s_instance_ = nullptr; }

void Application::close() { running_ = false; }

void Application::run() {
  PGL_INFO("Application created successfully");

  ui_layer_->init(window_->getNativeWindow());

  Renderer::Renderer::init();
  Renderer::Renderer::setClearColor({0.1f, 0.1f, 0.1f, 1.0f});

  Renderer::Camera camera(1280, 720);
  camera.setPosition({0.0f, 0.0f, 3.0f});

  // Test Triangle
  std::vector<float> vertices = {
      -0.5f, -0.5f, 0.0f, // left
      0.5f,  -0.5f, 0.0f, // right
      0.0f,  0.5f,  0.0f  // top
  };
  std::vector<uint32_t> indices = {0, 1, 2};
  Renderer::Mesh triangle(vertices, indices, {3});

  std::string vertexSrc = R"(
        #version 430 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 u_ViewProjection;
        uniform mat4 u_Transform;
        void main() {
            gl_Position = u_ViewProjection * u_Transform * vec4(aPos, 1.0);
        }
    )";

  std::string fragmentSrc = R"(
        #version 430 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.5, 0.2, 1.0);
        }
    )";
  Renderer::Shader rawShader(vertexSrc, fragmentSrc);

  // Particle Test Shader
  std::string pVertexSrc = R"(
        #version 430 core
        layout (location = 0) in vec3 aPos;
        // Instance attributes
        layout (location = 1) in vec3 aInstancePos;
        layout (location = 2) in float aInstanceScale;
        layout (location = 3) in vec4 aInstanceColor;

        uniform mat4 u_ViewProjection;
        out vec4 vColor;

        void main() {
            vColor = aInstanceColor;
            vec3 scaledPos = (aPos * aInstanceScale) + aInstancePos;
            gl_Position = u_ViewProjection * vec4(scaledPos, 1.0);
        }
    )";

  std::string pFragmentSrc = R"(
        #version 430 core
        in vec4 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vColor;
        }
    )";
  Renderer::Shader particleShader(pVertexSrc, pFragmentSrc);

  // ECS Setup
  ECS::Registry registry;
  ECS::Systems::ParticleSystem particleSystem;

  UI::ScenePanel scenePanel;
  scenePanel.setRegistry(&registry);
  UI::InspectorPanel inspectorPanel;
  inspectorPanel.setRegistry(&registry);
  UI::StatsPanel statsPanel;
  statsPanel.setRegistry(&registry);

  // Create emitter entity
  auto emitterEntity = registry.createEntity();
  registry.addComponent<ECS::Components::Transform>(
      emitterEntity, ECS::Components::Transform{
                         glm::vec3(0.0f, 0.0f, 0.0f),
                         glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)});

  registry.addComponent<ECS::Components::ParticleEmitter>(
      emitterEntity, ECS::Components::ParticleEmitter{
                         100.0f,                      // emit 100 per sec
                         0.0f,                        // timeSinceLastEmission
                         glm::vec3(0.0f, 2.0f, 0.0f), // initialVelocity
                         25.0f,                       // spreadAngle
                         glm::vec4(1.0f, 0.5f, 0.2f, 1.0f), // startColor
                         glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), // endColor
                         2.0f,                              // particleLifetime
                         5000,                              // up to 5000 max
                         0                                  // activeParticles
                     });
  registry.addComponent<ECS::Components::Lifetime>(emitterEntity,
                                                   ECS::Components::Lifetime{});

  float time_accumulator = 0.0f;

  while (running_) {
    float dt = 0.016f; // approximate step
    time_accumulator += dt;

    // 1. Sim Logic
    particleSystem.update(registry, dt);

    // 2. Render
    Renderer::Renderer::clear();

    ui_layer_->beginFrame();

    // Render UI Panels
    scenePanel.onImGuiRender();
    inspectorPanel.setSelectedEntity(scenePanel.getSelectedEntity());
    inspectorPanel.onImGuiRender();
    statsPanel.onImGuiRender();

    Renderer::Renderer::beginScene(camera);

    // Rendering only the particles in this test

    // Render particles
    auto &pools = particleSystem.getPools();
    for (const auto &[entity, pool] : pools) {
      // Un-const it for flush call
      auto &mutablePool = const_cast<Particles::ParticlePool &>(pool);
      mutablePool.flushToGPU();

      triangle.bind();
      mutablePool.getInstanceBuffer().linkToVao(1);
      triangle.unbind();

      Renderer::Renderer::drawInstanced(
          triangle, mutablePool.getInstanceBuffer(), particleShader);
    }

    Renderer::Renderer::endScene();

    ui_layer_->endFrame();

    window_->onUpdate();

    if (window_->shouldClose()) {
      running_ = false;
    }
  }

  ui_layer_->shutdown();
  Renderer::Renderer::shutdown();
}

} // namespace ParticleGL

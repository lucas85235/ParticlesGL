#include "Application.hpp"
#include "core/AssetManager.hpp"
#include "core/Logger.hpp"
#include "ecs/Registry.hpp"
#include "ecs/components/Lifetime.hpp"
#include "ecs/components/ParticleEmitter.hpp"
#include "ecs/components/Renderable.hpp"
#include "ecs/components/Transform.hpp"
#include "ecs/systems/ParticleSystem.hpp"
#include "particles_v2/ParticlePoolComponent.hpp"
#include "particles_v2/ParticleRenderSystem.hpp"
#include "particles_v2/ParticleSimulationSystem.hpp"
#include "../serialization/SceneSerializer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/Framebuffer.hpp"
#include "renderer/GpuParticleBuffer.hpp"
#include "renderer/Material.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/Shader.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#include "../ui/AssetsPanel.hpp"
#include "../ui/InspectorPanel.hpp"
#include "../ui/MaterialsPanel.hpp"
#include "../ui/ScenePanel.hpp"
#include "../ui/StatsPanel.hpp"
#include "../ui/ViewportPanel.hpp"

#include <glad/glad.h>
#include <imgui.h>
#include <imgui_internal.h>

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

  // Framebuffer Setup
  Renderer::FramebufferSpecification fbSpec;
  fbSpec.width = 1280;
  fbSpec.height = 720;
  std::shared_ptr<Renderer::Framebuffer> framebuffer =
      std::make_shared<Renderer::Framebuffer>(fbSpec);

  // We no longer strictly tie the window resize to the camera aspect ratio
  // directly, since the scene view is contained within the ViewportPanel. We
  // keep the window resize to update main viewport if needed.
  window_->setEventCallback(
      [](uint32_t width, uint32_t height) { glViewport(0, 0, width, height); });

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
  auto particleShaderPtr = Renderer::Shader::loadFromFile(
      "assets/shaders/particle.vert", "assets/shaders/particle.frag");
  if (!particleShaderPtr) {
    PGL_ERROR("Failed to load particle shader from disk!");
    return;
  }
  Renderer::Shader &particleShader = *particleShaderPtr;

  Core::AssetManager::init();

  // ECS Setup
  ECS::Registry registry;
  ECS::Systems::ParticleSimulationSystem particleSimulationSystem;
  ECS::Systems::ParticleRenderSystem particleRenderSystem;

  UI::ScenePanel scenePanel;
  scenePanel.setRegistry(&registry);
  UI::InspectorPanel inspectorPanel;
  inspectorPanel.setRegistry(&registry);
  UI::MaterialsPanel materialsPanel;
  materialsPanel.setRegistry(&registry);
  UI::StatsPanel statsPanel;
  statsPanel.setRegistry(&registry);
  UI::ViewportPanel viewportPanel;
  viewportPanel.setResizeCallback(
      [&camera, framebuffer](uint32_t width, uint32_t height) {
        framebuffer->resize(width, height);
        camera.setViewportSize(width, height);
      });

  UI::AssetsPanel assetsPanel;
  assetsPanel.setRegistry(&registry);
  assetsPanel.setAssetsRoot("assets");

  // Phase 4: Global GPU-driven particle pool (max 1M particles, 128 emitters).
  auto globalGpuBuf = std::make_shared<Renderer::GpuParticleBuffer>(1000000, 128);
  particleSimulationSystem.setGpuBuffer(globalGpuBuf.get());
  particleRenderSystem.setGpuBuffer(globalGpuBuf.get());
  scenePanel.setGpuBuffer(globalGpuBuf.get());

  auto mesh = Core::AssetManager::getDefaultMesh();
  uint32_t indexCount = mesh ? mesh->getIndexCount() : 36; // fallback for cube

  // Helper lambda to create an emitter
  auto spawnEmitterTest = [&](glm::vec3 pos, glm::vec4 startColor, glm::vec4 endColor, float speed) {
    auto entity = registry.createEntity();
    registry.addComponent<ECS::Components::Transform>(entity, ECS::Components::Transform{
                                                       pos, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)});

    uint32_t emitterIdx = 0, poolOffset = 0;
    uint32_t maxParticles = 50000; // Let's use larger emitters! 50k each
    if (globalGpuBuf->allocateEmitter(maxParticles, emitterIdx, poolOffset)) {
      globalGpuBuf->initDrawCommand(emitterIdx, indexCount, poolOffset);
      
      registry.addComponent<ECS::Components::ParticlePoolComponent>(entity, ECS::Components::ParticlePoolComponent{
          emitterIdx, poolOffset, maxParticles, 0, 0.0f
      });

      registry.addComponent<ECS::Components::ParticleEmitter>(
          entity, ECS::Components::ParticleEmitter{
                             20000.0f,                    // emit 20k per sec
                             0.0f,                        // timeSinceLastEmission
                             glm::vec3(0.0f, speed, 0.0f),// initialVelocity
                             35.0f,                       // spreadAngle
                             startColor,                  // startColor
                             endColor,                    // endColor
                             2.5f,                        // particleLifetime
                             maxParticles,                // max particles
                             0                            // activeParticles
                         });
      registry.addComponent<ECS::Components::Lifetime>(entity, ECS::Components::Lifetime{});
    }
  };

  // Emitter 1 (Fire)
  spawnEmitterTest(glm::vec3(-15.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.2f, 0.0f, 1.0f), glm::vec4(0.2f, 0.0f, 0.0f, 0.0f), 15.0f);
  // Emitter 2 (Magic)
  spawnEmitterTest(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(0.2f, 0.5f, 1.0f, 1.0f), glm::vec4(0.0f, 0.1f, 0.5f, 0.0f), 20.0f);
  // Emitter 3 (Poison)
  spawnEmitterTest(glm::vec3(15.0f, 0.0f, 0.0f), glm::vec4(0.2f, 1.0f, 0.2f, 1.0f), glm::vec4(0.0f, 0.2f, 0.0f, 0.0f), 10.0f);

  float time_accumulator = 0.0f;

  while (running_) {
    float dt = 0.016f; // approximate step
    time_accumulator += dt;

    // Apply any deferred framebuffer resize from the previous frame's UI pass.
    // Must happen before framebuffer bind to guarantee the FBO texture
    // submitted to ImGui::Image is always the current valid one.
    viewportPanel.applyPendingResize();

    // Sim Logic
    particleSimulationSystem.update(registry, dt);

    // Wait until beginFrame to draw UI Context
    ui_layer_->beginFrame();

    // Clear the default framebuffer for the main window background
    Renderer::Renderer::clear();

    // Enable Dockspace over the entire base window
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);

    static bool first_time = true;
    if (first_time) {
      first_time = false;
      // Only rebuild the dockspace layout if it's not already loaded from
      // imgui.ini
      if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id,
                                  ImGuiDockNodeFlags_PassthruCentralNode |
                                      ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id,
                                      ImGui::GetMainViewport()->Size);

        auto dock_id_scene = ImGui::DockBuilderSplitNode(
            dockspace_id, ImGuiDir_Left, 0.20f, nullptr, &dockspace_id);
        auto dock_id_inspector = ImGui::DockBuilderSplitNode(
            dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);
        auto dock_id_stats =
            ImGui::DockBuilderSplitNode(dock_id_inspector, ImGuiDir_Down, 0.30f,
                                        nullptr, &dock_id_inspector);
        // Assets panel splits the bottom of the left/scene column
        auto dock_id_assets = ImGui::DockBuilderSplitNode(
            dock_id_scene, ImGuiDir_Down, 0.40f, nullptr, &dock_id_scene);

        ImGui::DockBuilderDockWindow("Scene", dock_id_scene);
        ImGui::DockBuilderDockWindow("Assets", dock_id_assets);
        ImGui::DockBuilderDockWindow("Materials", dock_id_assets);
        ImGui::DockBuilderDockWindow("Inspector", dock_id_inspector);
        ImGui::DockBuilderDockWindow("Stats", dock_id_stats);
        ImGui::DockBuilderDockWindow("Viewport", dockspace_id);

        ImGui::DockBuilderFinish(dockspace_id);
      }
    }

    // Render Scene into Framebuffer
    framebuffer->bind();
    Renderer::Renderer::clear();
    Renderer::Renderer::beginScene(camera);

    // Render standard entities with Renderable
    auto renderables = registry.getEntitiesWith<ECS::Components::Renderable>();
    for (auto entity : renderables) {
      if (registry.hasComponent<ECS::Components::Transform>(entity)) {
        auto &transform =
            registry.getComponent<ECS::Components::Transform>(entity);
        auto &renderable =
            registry.getComponent<ECS::Components::Renderable>(entity);

        auto mesh = Core::AssetManager::getMesh(renderable.meshPath);
        auto material = Core::AssetManager::getMaterial(renderable.materialId);

        if (mesh && material) {
          material->bind();
          Renderer::Renderer::draw(*mesh, *material->shader,
                                   transform.matrix());
          material->unbind();
        }
      }
    }

    // Render particles
    particleRenderSystem.render(registry, particleShader);

    Renderer::Renderer::endScene();
    framebuffer->unbind();

    // Render UI Panels
    scenePanel.onImGuiRender();
    auto selectedEntity = scenePanel.getSelectedEntity();
    inspectorPanel.setSelectedEntity(selectedEntity);
    inspectorPanel.onImGuiRender();
    assetsPanel.setSelectedEntity(selectedEntity);
    assetsPanel.onImGuiRender();
    materialsPanel.setSelectedEntity(selectedEntity);
    materialsPanel.onImGuiRender();
    statsPanel.onImGuiRender();
    viewportPanel.onImGuiRender(framebuffer->getColorAttachmentRendererID());

    ui_layer_->endFrame();

    window_->onUpdate();

    if (window_->shouldClose()) {
      running_ = false;
    }
  }

  Core::AssetManager::shutdown();
  ui_layer_->shutdown();
  Renderer::Renderer::shutdown();
}

} // namespace ParticleGL

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
#include "renderer/Camera.hpp"
#include "renderer/Framebuffer.hpp"
#include "renderer/Material.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/PersistentInstanceBuffer.hpp"
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

  // Create emitter entity
  auto emitterEntity = registry.createEntity();
  registry.addComponent<ECS::Components::Transform>(
      emitterEntity, ECS::Components::Transform{
                         glm::vec3(0.0f, 0.0f, 0.0f),
                         glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)});

  // Phase 2: PersistentInstanceBuffer — 3 VBOs mapped directly into GPU memory.
  // The simulation writes position/scale/color straight through the mapped
  // pointers.
  std::vector<std::unique_ptr<Renderer::PersistentInstanceBuffer>> appBuffers;
  appBuffers.push_back(
      std::make_unique<Renderer::PersistentInstanceBuffer>(5000));

  Particles::ParticlePoolComponent pool;
  pool.init(5000);
  pool.gpu_buffer = appBuffers.back().get();
  registry.addComponent<Particles::ParticlePoolComponent>(emitterEntity,
                                                          std::move(pool));

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

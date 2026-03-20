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
#include "particles_v2/RadixSortSystem.hpp"
#include "ecs/systems/CameraControllerSystem.hpp"
#include "ecs/components/CameraController.hpp"
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

  // Main scene framebuffer
  Renderer::FramebufferSpecification fbSpec;
  fbSpec.width = 1280;
  fbSpec.height = 720;
  auto framebuffer = std::make_shared<Renderer::Framebuffer>(fbSpec);

  // Phase 5: Depth pre-pass framebuffer (depth-only, used for particle collision)
  Renderer::FramebufferSpecification depthFbSpec;
  depthFbSpec.width  = 1280;
  depthFbSpec.height = 720;
  depthFbSpec.depth_only = true;
  auto depthFramebuffer = std::make_shared<Renderer::Framebuffer>(depthFbSpec);

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
  ECS::Systems::RadixSortSystem radixSortSystem;
  ECS::Systems::CameraControllerSystem cameraControllerSystem;

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
      [&camera, framebuffer, depthFramebuffer](uint32_t width, uint32_t height) {
        framebuffer->resize(width, height);
        depthFramebuffer->resize(width, height);
        camera.setViewportSize(width, height);
      });

  UI::AssetsPanel assetsPanel;
  assetsPanel.setRegistry(&registry);
  assetsPanel.setAssetsRoot("assets");

  // Phase 4: Global GPU-driven particle pool (max 1M particles, 128 emitters).
  auto globalGpuBuf = std::make_shared<Renderer::GpuParticleBuffer>(1000000, 128);
  particleSimulationSystem.setGpuBuffer(globalGpuBuf.get());
  particleRenderSystem.setGpuBuffer(globalGpuBuf.get());
  radixSortSystem.setGpuBuffer(globalGpuBuf.get());
  scenePanel.setGpuBuffer(globalGpuBuf.get());

  // Set up camera entity with controller
  ECS::Entity cameraEntity = registry.createEntity();
  registry.addComponent<ECS::Components::Transform>(cameraEntity, ECS::Components::Transform{
      camera.getPosition(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)
  });
  registry.addComponent<ECS::Components::CameraController>(cameraEntity, ECS::Components::CameraController{
      &camera, 5.0f, 0.2f, false, false, 0.0f, 0.0f
  });

  // Helper lambda to create an emitter
  auto spawnEmitterTest = [&](glm::vec3 pos, glm::vec4 startColor, glm::vec4 endColor, float speed, uint32_t maxP, float emitRate) -> ECS::Entity {
    auto mesh = Core::AssetManager::getDefaultMesh();
    uint32_t indexCount = mesh ? mesh->getIndexCount() : 36; // fallback for cube
    
    auto entity = registry.createEntity();
    registry.addComponent<ECS::Components::Transform>(entity, ECS::Components::Transform{
                                                       pos, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)});

    uint32_t emitterIdx = 0, poolOffset = 0;
    if (globalGpuBuf->allocateEmitter(maxP, emitterIdx, poolOffset)) {
      globalGpuBuf->initDrawCommand(emitterIdx, indexCount, poolOffset);
      
      registry.addComponent<ECS::Components::ParticlePoolComponent>(entity, ECS::Components::ParticlePoolComponent{
          emitterIdx, poolOffset, maxP, 0, 0.0f
      });

      registry.addComponent<ECS::Components::ParticleEmitter>(
          entity, ECS::Components::ParticleEmitter{
                             emitRate,                    // emitRate
                             0.0f,                        // timeSinceLastEmission
                             glm::vec3(0.0f, speed, 0.0f),// initialVelocity
                             35.0f,                       // spreadAngle
                             startColor,                  // startColor
                             endColor,                    // endColor
                             2.5f,                        // particleLifetime
                             maxP,                        // max particles
                             0                            // activeParticles
                         });
      registry.addComponent<ECS::Components::Lifetime>(entity, ECS::Components::Lifetime{});
    }
    return entity;
  };

  // 1. Sparks (Child Emitter)
  ECS::Entity sparks = spawnEmitterTest(glm::vec3(0), 
      glm::vec4(1.0f, 0.8f, 0.1f, 1.0f), glm::vec4(1.0f, 0.1f, 0.0f, 0.0f), 
      0.0f, 200000, 0.0f); // 0 emission rate, pure sub-emitter
  
  auto& sparkEmitter = registry.getComponent<ECS::Components::ParticleEmitter>(sparks);
  sparkEmitter.particleLifetime = 1.0f;
  sparkEmitter.spreadAngle = 180.0f; // omnidirectional explosion
  sparkEmitter.bounciness = 0.3f;
  sparkEmitter.blendMode = ECS::Components::ParticleBlendMode::Additive;

  // 2. Rockets (Parent Emitter)
  ECS::Entity rockets = spawnEmitterTest(glm::vec3(0.0f, -5.0f, 0.0f), 
      glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.1f, 0.3f, 1.0f, 0.0f), 
      25.0f, 1000, 5.0f); // 5 rockets per second
  
  auto& rocketEmitter = registry.getComponent<ECS::Components::ParticleEmitter>(rockets);
  rocketEmitter.particleLifetime = 1.5f;
  rocketEmitter.spreadAngle = 10.0f;
  
  // Link Sub-Emitter
  rocketEmitter.subEmitterEnabled = true;
  rocketEmitter.childEmitterEntity = sparks;
  rocketEmitter.spawnCountOnDeath = 150; // 150 sparks per firework
  rocketEmitter.childSpeedScale = 0.3f;  // Inherit 30% of rocket velocity

  float time_accumulator = 0.0f;

  // GPU Timer Queries (Ping-Pong buffers to avoid pipeline stall)
  GLuint gpu_queries[2];
  glGenQueries(2, gpu_queries);
  int query_front = 0, query_back = 1;

  while (running_) {
    float dt = 0.016f; // approximate step
    time_accumulator += dt;

    // Apply any deferred framebuffer resize from the previous frame's UI pass.
    // Must happen before framebuffer bind to guarantee the FBO texture
    // submitted to ImGui::Image is always the current valid one.
    viewportPanel.applyPendingResize();

    // Begin GPU Time measurement for the current frame
    glBeginQuery(GL_TIME_ELAPSED, gpu_queries[query_back]);

    // Wait until beginFrame to draw UI Context, which also updates ImGui input states
    ui_layer_->beginFrame();

    // Update Camera hover state and controller
    if (registry.hasComponent<ECS::Components::CameraController>(cameraEntity)) {
        auto& camCtrl = registry.getComponent<ECS::Components::CameraController>(cameraEntity);
        camCtrl.isViewportHovered = viewportPanel.isHovered();
    }
    cameraControllerSystem.update(registry, dt);

    // Sim Logic: feed camera state before dispatching compute shaders
    particleSimulationSystem.setCamera(camera.getViewMatrix(), camera.getProjectionMatrix());
    particleSimulationSystem.setSceneDepthTexture(depthFramebuffer->getDepthAttachmentRendererID());
    particleSimulationSystem.update(registry, dt);

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

    // ── Depth Pre-Pass ─────────────────────────────────────────────────────
    // Render opaque geometry into the depth-only FBO so the compute shader
    // can perform depth-based scene collision.
    depthFramebuffer->bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    Renderer::Renderer::beginScene(camera);
    for (auto entity : registry.getEntitiesWith<ECS::Components::Renderable>()) {
      if (registry.hasComponent<ECS::Components::Transform>(entity)) {
        auto &transform = registry.getComponent<ECS::Components::Transform>(entity);
        auto &renderable = registry.getComponent<ECS::Components::Renderable>(entity);
        auto mesh = Core::AssetManager::getMesh(renderable.meshPath);
        auto material = Core::AssetManager::getMaterial(renderable.materialId);
        if (mesh && material) {
          Renderer::Renderer::draw(*mesh, *material->shader, transform.matrix());
        }
      }
    }
    Renderer::Renderer::endScene();
    depthFramebuffer->unbind();

    // ── Main Scene Render ───────────────────────────────────────────────────
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

    // Sort Alpha-blend emitters back-to-front (GPU Radix Sort)
    radixSortSystem.setCameraPosition(camera.getPosition());
    radixSortSystem.sort(registry);

    // Render particles
    particleRenderSystem.render(registry, particleShader);

    Renderer::Renderer::endScene();
    framebuffer->unbind();

    // End GPU Time measurement
    glEndQuery(GL_TIME_ELAPSED);

    // Query the front buffer (previous frame) to see if it's available, without blocking
    GLuint available = 0;
    glGetQueryObjectuiv(gpu_queries[query_front], GL_QUERY_RESULT_AVAILABLE, &available);
    if (available) {
        GLuint64 elapsed_ns = 0;
        glGetQueryObjectui64v(gpu_queries[query_front], GL_QUERY_RESULT, &elapsed_ns);
        float gpu_time_ms = static_cast<float>(elapsed_ns) / 1000000.0f;
        statsPanel.setGpuTimeMs(gpu_time_ms);
    }
    
    // Pass other metrics to StatsPanel
    statsPanel.setCpuTimeMs(particleSimulationSystem.getCpuSimulateTimeMs());
    statsPanel.setDrawCalls(Renderer::Renderer::getDrawCalls());
    
    // Reset Renderer metrics for next frame
    Renderer::Renderer::resetStats();

    // Swap GPU queries
    std::swap(query_front, query_back);

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

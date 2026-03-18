#include <glad/glad.h>

#include "ComputeApp.hpp"
#include "core/Logger.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

namespace ParticleGL::Demo {

ComputeApp::ComputeApp() {
  PGL_INFO("Compute Demo starting...");

  WindowProps props;
  props.title = "ParticleGL - Compute Shader Demo";
  props.width = 1280;
  props.height = 720;
  window_ = std::make_unique<Window>(props);

  window_->setEventCallback([this](uint32_t width, uint32_t height) {
    // Basic resize event handling could go here
  });

  // Init ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL((GLFWwindow *)window_->getNativeWindow(), true);
  ImGui_ImplOpenGL3_Init("#version 410");

  if (!computeExample_.init("assets/shaders/color_wave.comp")) {
    PGL_ERROR("Failed to initialize compute example");
  }
}

ComputeApp::~ComputeApp() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ComputeApp::run() {
  float time_accumulator = 0.0f;

  while (running_) {
    // Assuming 60fps fixed step for demo purposes
    float dt = 0.016f;
    time_accumulator += dt;

    window_->onUpdate();

    if (((GLFWwindow *)window_->getNativeWindow()) == nullptr ||
        glfwWindowShouldClose((GLFWwindow *)window_->getNativeWindow())) {
      running_ = false;
    }

    // 1. Tick compute shader
    computeExample_.tick(time_accumulator);

    // 2. Render UI
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Compute Shader Output");
    ImGui::Text("GPU computed color wave (1024 elements)");
    ImGui::Text("Dispatching 16 work groups of 64 threads each frame.");

    glm::vec4 color = computeExample_.getSampleColor();
    ImGui::ColorButton("##compute_color",
                       ImVec4(color.r, color.g, color.b, color.a),
                       ImGuiColorEditFlags_NoPicker, ImVec2(100.0f, 100.0f));
    ImGui::Text("Buffer[0] = (%.2f, %.2f, %.2f)", color.r, color.g, color.b);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow *backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }
  }
}

} // namespace ParticleGL::Demo

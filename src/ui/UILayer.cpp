#include "UILayer.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace ParticleGL::UI {

void UILayer::init(GLFWwindow *window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui::StyleColorsDark();
  setDarkThemeColors();

  // Initialize backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 430");
}

void UILayer::shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void UILayer::beginFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void UILayer::endFrame() {
  ImGuiIO &io = ImGui::GetIO();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  // Handle multi-viewport for docking
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow *backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
  }
}

void UILayer::setDarkThemeColors() {
  auto &style = ImGui::GetStyle();
  auto &colors = style.Colors;

  // Modern Window rounding & styling
  style.WindowRounding = 5.0f;
  style.FrameRounding = 3.0f;
  style.PopupRounding = 3.0f;
  style.TabRounding = 2.0f;
  style.ScrollbarRounding = 9.0f;
  style.GrabRounding = 3.0f;

  style.WindowPadding = ImVec2(8.0f, 8.0f);
  style.FramePadding = ImVec2(6.0f, 4.0f);
  style.ItemSpacing = ImVec2(8.0f, 4.0f);

  // Core Backgrounds
  colors[ImGuiCol_WindowBg] = ImVec4{0.10f, 0.105f, 0.11f, 1.0f};
  colors[ImGuiCol_ChildBg] = ImVec4{0.12f, 0.125f, 0.13f, 1.0f};
  colors[ImGuiCol_PopupBg] = ImVec4{0.11f, 0.115f, 0.12f, 1.0f};

  // Borders
  colors[ImGuiCol_Border] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
  colors[ImGuiCol_BorderShadow] = ImVec4{0.0f, 0.0f, 0.0f, 0.0f};

  // Headers (Trees, CollapsingHeaders, Selectables)
  colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
  colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
  colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

  // Buttons
  colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
  colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
  colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

  // Frame BG (Inputs, Checkboxes)
  colors[ImGuiCol_FrameBg] = ImVec4{0.16f, 0.165f, 0.17f, 1.0f};
  colors[ImGuiCol_FrameBgHovered] = ImVec4{0.23f, 0.235f, 0.24f, 1.0f};
  colors[ImGuiCol_FrameBgActive] = ImVec4{0.20f, 0.205f, 0.21f, 1.0f};

  // Tabs
  colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
  colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
  colors[ImGuiCol_TabSelected] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
  colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

  // Title
  colors[ImGuiCol_TitleBg] = ImVec4{0.10f, 0.105f, 0.11f, 1.0f};
  colors[ImGuiCol_TitleBgActive] = ImVec4{0.12f, 0.125f, 0.13f, 1.0f};
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.10f, 0.105f, 0.11f, 1.0f};

  // Separators and Resizers
  colors[ImGuiCol_SeparatorHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
  colors[ImGuiCol_SeparatorActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
  colors[ImGuiCol_ResizeGrip] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
  colors[ImGuiCol_ResizeGripHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
  colors[ImGuiCol_ResizeGripActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

  // Docking
  colors[ImGuiCol_DockingPreview] = ImVec4{0.38f, 0.3805f, 0.381f, 0.701f};
  colors[ImGuiCol_DockingEmptyBg] = ImVec4{0.10f, 0.105f, 0.11f, 1.0f};
}

} // namespace ParticleGL::UI

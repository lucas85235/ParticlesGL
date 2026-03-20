#include "StatsPanel.hpp"
#include "../ecs/components/ParticleEmitter.hpp"
#include <glm/glm.hpp>
#include <imgui.h>

namespace ParticleGL::UI {

void StatsPanel::onImGuiRender() {
  ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

  ImGuiIO &io = ImGui::GetIO();
  
  // Section: Engine
  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Engine Status");
  ImGui::Separator();
  ImGui::Text("FPS:           %.1f", io.Framerate);
  ImGui::Text("Frame Time:    %.3f ms", 1000.0f / io.Framerate);
  ImGui::Spacing();
  ImGui::Spacing();

  if (registry_) {
    int total_active_particles = 0;
    int total_emitters = 0;

    // Sum active particles from all emitters
    auto emitters =
        registry_->getEntitiesWith<ECS::Components::ParticleEmitter>();
    for (auto entity : emitters) {
      if (registry_->hasComponent<ECS::Components::ParticleEmitter>(entity)) {
        auto &emitter =
            registry_->getComponent<ECS::Components::ParticleEmitter>(entity);
        total_active_particles += emitter.activeParticles;
        total_emitters++;
      }
    }

    // Section: System
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Particle System");
    ImGui::Separator();
    ImGui::Text("Emitters Count:         %d", total_emitters);
    ImGui::Text("Total Live Particles:   %d", total_active_particles);
    ImGui::Text("Draw Calls (Frame):     %u", draw_calls_);
    ImGui::Spacing();
    ImGui::Spacing();

    // Section: Timings
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "Performance Timings");
    ImGui::Separator();
    
    // Color code frame times: < 8ms = Green, 8-16ms = Yellow, > 16ms = Red
    ImVec4 cpuColor = cpu_time_ms_ < 8.0f ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : (cpu_time_ms_ < 16.0f ? ImVec4(1.0f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    ImVec4 gpuColor = gpu_time_ms_ < 8.0f ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : (gpu_time_ms_ < 16.0f ? ImVec4(1.0f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    
    ImGui::Text("CPU Simulate Time:   "); ImGui::SameLine(180); ImGui::TextColored(cpuColor, "%8.3f ms", cpu_time_ms_);
    ImGui::Text("GPU Frame Time:      "); ImGui::SameLine(180); ImGui::TextColored(gpuColor, "%8.3f ms", gpu_time_ms_);
    ImGui::Spacing();
    ImGui::Spacing();

    // Section: Bandwidth/Data
    ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "Data & Bandwidth");
    ImGui::Separator();
    
    // Estimate bandwidth: position(16) + color(16) + vel(16) + life(8) = ~56 bytes per particle roundtrip, theoretically.
    float theoretical_bandwidth = (total_active_particles * 64 * io.Framerate) / (1024.0f * 1024.0f);
    float vram_usage = (total_active_particles * 64.0f) / (1024.0f * 1024.0f);
    
    ImGui::Text("Upload CPU->GPU:     "); ImGui::SameLine(180); ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "0.00 MB/s"); 
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Because we use Compute Shaders (Phase 6), particles live exclusively on VRAM.\nUpload bandwidth is effectively zero.");
    }
    
    ImGui::TextDisabled("Theoretical Load:    "); ImGui::SameLine(180); ImGui::TextDisabled("%8.2f MB/s", theoretical_bandwidth);
    ImGui::Text("Est. VRAM Usage:     "); ImGui::SameLine(180); ImGui::Text("%8.2f MB", vram_usage);
  }

  ImGui::End();
}


} // namespace ParticleGL::UI

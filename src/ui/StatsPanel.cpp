#include "StatsPanel.hpp"
#include "../ecs/components/ParticleEmitter.hpp"
#include <glm/glm.hpp>
#include <imgui.h>

namespace ParticleGL::UI {

void StatsPanel::onImGuiRender() {
  ImGui::Begin("Statistics");

  ImGuiIO &io = ImGui::GetIO();
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / io.Framerate, io.Framerate);

  if (registry_) {
    int total_active_particles = 0;

    // Sum active particles from all emitters
    auto emitters =
        registry_->getEntitiesWith<ECS::Components::ParticleEmitter>();
    for (auto entity : emitters) {
      if (registry_->hasComponent<ECS::Components::ParticleEmitter>(entity)) {
        auto &emitter =
            registry_->getComponent<ECS::Components::ParticleEmitter>(entity);
        total_active_particles += emitter.activeParticles;
      }
    }

    ImGui::Text("Total Active Particles: %d", total_active_particles);
  }

  ImGui::Separator();
  ImGui::Text("Compute Shader Demo");
  // Display a live colored swatch updated by the compute shader each frame
  ImGui::ColorButton("##compute_sample",
                     ImVec4(compute_sample_color_.r, compute_sample_color_.g,
                            compute_sample_color_.b, compute_sample_color_.a),
                     ImGuiColorEditFlags_NoPicker, ImVec2(14.0f, 14.0f));
  ImGui::SameLine();
  ImGui::Text("GPU particle[0] color (%.2f, %.2f, %.2f)",
              compute_sample_color_.r, compute_sample_color_.g,
              compute_sample_color_.b);

  ImGui::End();
}

} // namespace ParticleGL::UI

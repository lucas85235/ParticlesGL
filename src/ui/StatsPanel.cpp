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

  ImGui::End();
}

} // namespace ParticleGL::UI

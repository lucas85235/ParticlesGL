#include "SceneSerializer.hpp"
#include "../core/AssetManager.hpp"
#include "../core/Logger.hpp"
#include "ComponentSerializer.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

namespace ParticleGL::Serialization {

using nlohmann::json;
using namespace ECS;

void SceneSerializer::serialize(const std::string &filepath) {
  json scene_json;
  auto entities = registry_->getEntities();

  json entities_json = json::array();
  for (Entity entity : entities) {
    json entity_json;
    entity_json["id"] = entity;

    if (registry_->hasComponent<Components::Transform>(entity)) {
      entity_json["Transform"] =
          registry_->getComponent<Components::Transform>(entity);
    }
    if (registry_->hasComponent<Components::ParticleEmitter>(entity)) {
      entity_json["ParticleEmitter"] =
          registry_->getComponent<Components::ParticleEmitter>(entity);
    }
    if (registry_->hasComponent<Components::Lifetime>(entity)) {
      entity_json["Lifetime"] =
          registry_->getComponent<Components::Lifetime>(entity);
    }
    if (registry_->hasComponent<Components::Renderable>(entity)) {
      entity_json["Renderable"] =
          registry_->getComponent<Components::Renderable>(entity);
    }

    entities_json.push_back(entity_json);
  }
  scene_json["entities"] = entities_json;

  json materials_json = json::array();
  for (const auto &[name, material] : Core::AssetManager::getMaterials()) {
    json mat_json;
    mat_json["id"] = material->id;
    mat_json["shaderId"] = material->shaderId;
    mat_json["baseColor"] = {material->baseColor.r, material->baseColor.g,
                             material->baseColor.b, material->baseColor.a};
    materials_json.push_back(mat_json);
  }
  scene_json["materials"] = materials_json;

  std::ofstream out(filepath);
  out << scene_json.dump(4);
}

bool SceneSerializer::deserialize(const std::string &filepath) {
  std::ifstream in(filepath);
  if (!in.is_open())
    return false;

  json scene_json;
  in >> scene_json;

  auto entities = registry_->getEntities();
  for (Entity entity : entities) {
    registry_->destroyEntity(entity);
  }
  if (scene_json.contains("materials")) {
    for (const auto &mat_json : scene_json["materials"]) {
      std::string id = mat_json["id"];
      std::string shaderId = mat_json["shaderId"];
      auto shader = Core::AssetManager::getShader(shaderId);
      if (shader) {
        auto mat = std::make_shared<Renderer::Material>(id, shaderId, shader);
        if (mat_json.contains("baseColor")) {
          auto bc = mat_json["baseColor"];
          if (bc.size() == 4) {
            mat->baseColor = glm::vec4(bc[0], bc[1], bc[2], bc[3]);
          }
        }
        Core::AssetManager::addMaterial(id, mat);
      } else {
        PGL_ERROR("Failed to load shader '" << shaderId << "' for material '"
                                            << id << "'");
      }
    }
  }

  if (scene_json.contains("entities")) {
    for (const auto &entity_json : scene_json["entities"]) {
      Entity entity = registry_->createEntity();

      if (entity_json.contains("Transform")) {
        registry_->addComponent(
            entity, entity_json["Transform"].get<Components::Transform>());
      }
      if (entity_json.contains("ParticleEmitter")) {
        registry_->addComponent(
            entity,
            entity_json["ParticleEmitter"].get<Components::ParticleEmitter>());
      }
      if (entity_json.contains("Lifetime")) {
        registry_->addComponent(
            entity, entity_json["Lifetime"].get<Components::Lifetime>());
      }
      if (entity_json.contains("Renderable")) {
        registry_->addComponent(
            entity, entity_json["Renderable"].get<Components::Renderable>());
      }
    }
  }
  return true;
}

} // namespace ParticleGL::Serialization

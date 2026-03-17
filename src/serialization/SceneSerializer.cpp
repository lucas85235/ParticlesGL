#include "SceneSerializer.hpp"
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

    entities_json.push_back(entity_json);
  }

  scene_json["entities"] = entities_json;

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
    }
  }
  return true;
}

} // namespace ParticleGL::Serialization

#include "Registry.hpp"
#include <algorithm>

namespace ParticleGL::ECS {

Entity Registry::createEntity() {
  if (!free_entities_.empty()) {
    Entity entity = free_entities_.back();
    free_entities_.pop_back();
    return entity;
  }
  return next_entity_id_++;
}

void Registry::destroyEntity(Entity entity) {
  // Clean up all components
  for (auto &[type, pool] : component_pools_) {
    pool->removeEntity(entity);
  }

  // Recycle the Entity ID
  free_entities_.push_back(entity);
}

std::vector<Entity> Registry::getEntities() const {
  std::vector<Entity> entities;
  entities.reserve(next_entity_id_ - free_entities_.size());
  for (Entity i = 0; i < next_entity_id_; ++i) {
    if (std::find(free_entities_.begin(), free_entities_.end(), i) ==
        free_entities_.end()) {
      entities.push_back(i);
    }
  }
  return entities;
}

} // namespace ParticleGL::ECS

#include "Registry.hpp"

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

} // namespace ParticleGL::ECS

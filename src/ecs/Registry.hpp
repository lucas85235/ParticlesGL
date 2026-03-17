#pragma once

#include "ComponentPool.hpp"
#include "Entity.hpp"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ParticleGL::ECS {

class Registry {
public:
  Registry() = default;
  ~Registry() = default;

  // Entity Management
  Entity createEntity();
  void destroyEntity(Entity entity);

  // Component Management
  template <typename T> void addComponent(Entity entity, T component) {
    getComponentPool<T>()->insert(entity, std::move(component));
  }

  template <typename T> void removeComponent(Entity entity) {
    getComponentPool<T>()->remove(entity);
  }

  template <typename T> bool hasComponent(Entity entity) const {
    std::type_index typeName = std::type_index(typeid(T));
    auto it = component_pools_.find(typeName);
    if (it == component_pools_.end()) {
      return false;
    }
    auto pool = static_cast<ComponentPool<T> *>(it->second.get());
    return pool->has(entity);
  }

  template <typename T> T &getComponent(Entity entity) {
    return getComponentPool<T>()->get(entity);
  }

  // Helper to fetch all entities that possess a specific component
  // Real sparse-set views do an intersection, here we just return the first
  // pool's entities and let the caller double check.
  template <typename T> std::vector<Entity> getEntitiesWith() {
    auto pool = getComponentPool<T>();
    const auto &indexToEntity = pool->getIndexToEntityMap();
    std::vector<Entity> entities;
    entities.reserve(indexToEntity.size());
    for (const auto &pair : indexToEntity) {
      entities.push_back(pair.second);
    }
    return entities;
  }

private:
  std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>>
      component_pools_;
  Entity next_entity_id_ = 0;
  std::vector<Entity> free_entities_;

  template <typename T> ComponentPool<T> *getComponentPool() {
    std::type_index typeName = std::type_index(typeid(T));
    if (component_pools_.find(typeName) == component_pools_.end()) {
      component_pools_[typeName] = std::make_unique<ComponentPool<T>>();
    }
    return static_cast<ComponentPool<T> *>(component_pools_[typeName].get());
  }
};

} // namespace ParticleGL::ECS

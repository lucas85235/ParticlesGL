#pragma once

#include "Entity.hpp"
#include <cassert>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace ParticleGL::ECS {

class IComponentPool {
public:
  virtual ~IComponentPool() = default;
  virtual void removeEntity(Entity entity) = 0;
};

template <typename T> class ComponentPool : public IComponentPool {
public:
  ComponentPool() = default;
  ~ComponentPool() override = default;

  T &insert(Entity entity, T component) {
    assert(entity_to_index_.find(entity) == entity_to_index_.end() &&
           "Entity already has this component!");

    std::size_t newIndex = components_.size();
    entity_to_index_[entity] = newIndex;
    index_to_entity_[newIndex] = entity;
    components_.push_back(std::move(component));

    return components_[newIndex];
  }

  T &get(Entity entity) {
    assert(entity_to_index_.find(entity) != entity_to_index_.end() &&
           "Entity does not have this component!");
    return components_[entity_to_index_[entity]];
  }

  void remove(Entity entity) {
    if (entity_to_index_.find(entity) == entity_to_index_.end()) {
      return;
    }

    std::size_t indexOfRemoved = entity_to_index_[entity];
    std::size_t indexOfLast = components_.size() - 1;

    if (indexOfRemoved != indexOfLast) {
      // Swap with the last element to maintain contiguous memory
      components_[indexOfRemoved] = std::move(components_[indexOfLast]);

      // Update the index mappings for the swapped element
      Entity entityOfLast = index_to_entity_[indexOfLast];
      entity_to_index_[entityOfLast] = indexOfRemoved;
      index_to_entity_[indexOfRemoved] = entityOfLast;
    }

    entity_to_index_.erase(entity);
    index_to_entity_.erase(indexOfLast);
    components_.pop_back();
  }

  void removeEntity(Entity entity) override { remove(entity); }

  bool has(Entity entity) const {
    return entity_to_index_.find(entity) != entity_to_index_.end();
  }

  std::size_t size() const { return components_.size(); }

  void clear() {
    components_.clear();
    entity_to_index_.clear();
    index_to_entity_.clear();
  }

  // Direct data access for iterating
  std::vector<T> &getData() { return components_; }
  const std::vector<T> &getData() const { return components_; }

  const std::unordered_map<std::size_t, Entity> &getIndexToEntityMap() const {
    return index_to_entity_;
  }

private:
  std::vector<T> components_;
  std::unordered_map<Entity, std::size_t> entity_to_index_;
  std::unordered_map<std::size_t, Entity> index_to_entity_;
};

} // namespace ParticleGL::ECS

# Entity Component System (ECS)

The engine implements a heavily custom **Entity Component System (ECS)** optimized with sparse-sets to ensure contiguous memory access while allowing O(1) constant-time component lookups. 

## Key Concepts

### 1. `Entity`
An `Entity` in this engine is merely an ID, represented by a `uint32_t` (`src/ecs/Entity.hpp`). An entity by itself contains no data or logic.

### 2. `ComponentPool`
All components of the same type are stored in a contiguous `std::vector` inside a `ComponentPool<T>`. To maintain `O(1)` retrieval, the pool uses a sparse array that maps an `Entity` ID to the dense array index where the component actually lives. When a component is destroyed, the last element in the dense array is swapped with the deleted element to preserve contiguity, and the sparse array is updated accordingly.

### 3. `Registry`
The `Registry` (`src/ecs/Registry.hpp`) manages the lifecycle of `Entity` IDs and acts as the central hub for accessing `ComponentPool`s. It creates entities, recycles destroyed IDs, and provides generic template functions to attach or retrieve components:
```cpp
auto entity = registry.createEntity();
registry.addComponent<Transform>(entity, Transform{...});
auto& transform = registry.getComponent<Transform>(entity);
```

### 4. Components
Components are pure **Plain Old Data (POD)** structs located in `src/ecs/components/`. 
- `Transform`: Handles 3D space location, rotation (quaternion), and scale. Computes the `matrix()` dynamically.
- `Renderable`: Determines visual representation (points to a `meshPath` and a `materialId`).
- `ParticleEmitter`: Configurations for emitting instances (rate, velocity, color gradients).
- `Lifetime`: Tracks component deletion timing (used by particles).

### 5. `System`
Systems contain the logic that iterates over specific components. Systems in ParticleGL inherit from the base `System` class and operate over all entities possessing the required components.
For instance, `ParticleSystem` requests all entities with `ParticleEmitter` components and ticks their emission state.

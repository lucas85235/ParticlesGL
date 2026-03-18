# particle_system

This directory represents the **Entity Component System (ECS) / Component-Driven Particle** architecture approach.

In contrast to `src/particles` — which ties a `ParticlePool` explicitly to an ECS Entity via an external `unordered_map` —
this approach would embed the particle pool **directly as a component** in the ECS registry.
The particle system then becomes a standard ECS System iterating over all `(ParticleEmitterComponent, ParticlePoolComponent)` pairs.

## Concept Overview

```
entity E  ->  [Transform]  [ParticleEmitterConfig]  [ParticlePoolComponent]
                                                           ^
                                                   stores SoA arrays + GPU VBO
```

The System accesses pool data via the registry's sparse-set iteration, eliminating the
external `std::unordered_map<Entity, ParticlePool>` coupling present in the current implementation.
This enables the ECS to manage pool lifetimes automatically (the pool destructs when the entity does),
and allows filters like `getEntitiesWith<ParticleEmitter, ParticlePool>()` to drive rendering without any indirection.

This directory would host a full standalone implementation of this approach. See `docs/particle_comparison.md` for a deep technical comparison.

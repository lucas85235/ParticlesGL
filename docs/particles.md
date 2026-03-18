# Particle System & GPU Instancing

The Particle System in `ParticleGL` is designed for high performance, simulating particles on the CPU inside contiguous arrays (pools) while drawing them in a single draw call via GPU Instancing.

## Core Concepts

### 1. `ParticleEmitter` Component
Any `Entity` can become a particle source by attaching the `ParticleEmitter` component. 

This component holds:
- **`emissionRate`**: Particles generated per second.
- **`initialVelocity` & `spreadAngle`**: Define the emission cone/direction randomly chosen per particle.
- **`startColor` & `endColor`**: Gradual interpolation over the particle's lifetime.
- **`particleLifetime` & `maxParticles`**: Defines pool size limitations.

### 2. `ParticlePool`
A `ParticlePool` (`src/particles/ParticlePool.hpp`) acts as the CPU-side data host. It statically pre-allocates vectors up to the emitter's `maxParticles`. 

- A `tail` index tracks active particles. 
- When a particle dies, it is swapped with the last active particle (O(1) kill) preserving array contiguity.
- Features an `InstanceBuffer` wrapper, exposing dynamic geometry data directly to OpenGL via `glBufferSubData`.

### 3. `ParticleSystem` Logic
Located at `src/ecs/systems/ParticleSystem.hpp`, this system ticks via the `update(dt)` loop.
1. It queries the `Registry` to find all active `ParticleEmitter`s and links them to unique `ParticlePool` instances.
2. It processes emission (spawning new particles if `timeSinceLastEmission >= 1.0f / emissionRate`).
3. It steps all active particles in the pools, interpolating their `vec4` color based on their remaining lifetime and applying gravity/velocity to their positions.

### 4. GPU Instancing
Every frame, `ParticleSystem::flushToGPU()` pushes the contiguous CPU array into the bounded OpenGL `InstanceBuffer` and triggers a single instanced rendering call (`Renderer::drawInstanced`). The `particle.vert` shader fetches the specific local attributes like `aInstancePos` and `aInstanceColor` utilizing layout divisor steps.

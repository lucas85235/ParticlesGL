#include "ParticleSimulationSystem.hpp"
#include "../ecs/components/ParticleEmitter.hpp"
#include "../ecs/components/Transform.hpp"
#include "ParticlePoolComponent.hpp"
#include "core/AssetManager.hpp"
#include "core/Logger.hpp"
#include "renderer/GpuParticleBuffer.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <vector>
#include <chrono>

namespace ParticleGL::ECS::Systems {

ParticleSimulationSystem::ParticleSimulationSystem() = default;

void ParticleSimulationSystem::lazyInit() {
  if (initialized_)
    return;

  simulate_shader_ = Renderer::ComputeShader::loadFromFile(
      "assets/shaders/particle_simulate.comp");
  compact_shader_ = Renderer::ComputeShader::loadFromFile(
      "assets/shaders/particle_compact.comp");
  emit_events_shader_ = Renderer::ComputeShader::loadFromFile(
      "assets/shaders/particle_emit_events.comp");
  spawn_from_events_shader_ = Renderer::ComputeShader::loadFromFile(
      "assets/shaders/particle_spawn_from_events.comp");

  if (!simulate_shader_ || !compact_shader_ || !emit_events_shader_ || !spawn_from_events_shader_) {
    PGL_ERROR("ParticleSimulationSystem: failed to load compute shaders.");
    return;
  }

  PGL_INFO("ParticleSimulationSystem: compute shaders loaded.");
  initialized_ = true;
}

void ParticleSimulationSystem::update(Registry &registry, float dt) {
  lazyInit();
  if (!initialized_)
    return;

  if (!gpu_buffer_)
    return;
  
  auto start_time = std::chrono::high_resolution_clock::now();

  auto &gpuBuf = *gpu_buffer_;

  auto emitters = registry.getEntitiesWith<ECS::Components::ParticleEmitter>();

  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<float> rand_spread(-1.0f, 1.0f);

  // 1. Spawning Phase
  for (Entity entity : emitters) {
    if (!registry.hasComponent<ECS::Components::Transform>(entity)) {
      continue;
    }

    if (!registry.hasComponent<ECS::Components::ParticlePoolComponent>(entity)) {
      auto &emitter = registry.getComponent<ECS::Components::ParticleEmitter>(entity);
      uint32_t emitterIdx = 0, poolOffset = 0;
      if (gpuBuf.allocateEmitter(emitter.maxParticles, emitterIdx, poolOffset)) {
          auto mesh = Core::AssetManager::getDefaultMesh();
          uint32_t indexCount = mesh ? mesh->getIndexCount() : 36;
          gpuBuf.initDrawCommand(emitterIdx, indexCount, poolOffset);

          registry.addComponent<ECS::Components::ParticlePoolComponent>(entity, ECS::Components::ParticlePoolComponent{
              emitterIdx, poolOffset, emitter.maxParticles, 0, 0.0f
          });
      } else {
          continue;
      }
    }

    auto &emitter = registry.getComponent<ECS::Components::ParticleEmitter>(entity);
    auto &transform = registry.getComponent<ECS::Components::Transform>(entity);
    auto &pool = registry.getComponent<ECS::Components::ParticlePoolComponent>(entity);

    // If unallocated, ignore for now (should be allocated by Application)
    if (pool.pool_size == 0) continue;

    float toSpawnF = dt * emitter.emissionRate + pool.emission_accumulator;
    uint32_t toSpawn = static_cast<uint32_t>(toSpawnF);
    pool.emission_accumulator = toSpawnF - static_cast<float>(toSpawn);

    uint32_t available = pool.pool_size > pool.active_count
                             ? pool.pool_size - pool.active_count
                             : 0;
    toSpawn = std::min(toSpawn, available);

    if (toSpawn > 0) {
      std::vector<Renderer::GpuParticleBuffer::SpawnData> batch(toSpawn);

      const float speed = glm::length(emitter.initialVelocity);
      const glm::vec3 baseDir = speed > 0.001f
                                    ? glm::normalize(emitter.initialVelocity)
                                    : glm::vec3(0.0f, 1.0f, 0.0f);

      for (uint32_t i = 0; i < toSpawn; ++i) {
        glm::vec3 spray(rand_spread(gen), rand_spread(gen), rand_spread(gen));
        glm::vec3 dir = baseDir + spray * (emitter.spreadAngle / 90.0f);
        dir = glm::length(dir) > 0.001f ? glm::normalize(dir) : baseDir;

        batch[i].position = glm::vec4(transform.position, 0.0f);
        batch[i].velocity = glm::vec4(dir * speed, 0.0f);
        batch[i].life = 0.0f;
        batch[i].maxLife = emitter.particleLifetime;
        batch[i].color = emitter.startColor;
        batch[i].startColor = emitter.startColor;
        batch[i].endColor = emitter.endColor;
        batch[i].scale = 1.0f;
      }

      gpuBuf.uploadSpawnBatch(pool.pool_offset, pool.active_count, batch.data(), toSpawn);
      pool.active_count += toSpawn;
      gpuBuf.setActiveCount(pool.emitter_index, pool.active_count);
    }
  }

  gpuBuf.bindSsbos();

  // 2. Simulation Phase
  simulate_shader_->bind();
  simulate_shader_->setFloat("u_dt", dt);
  simulate_shader_->setVec3("u_gravity", glm::vec3(0.0f, -9.81f, 0.0f));
  simulate_shader_->setFloat("u_time", elapsed_time_);
  simulate_shader_->setMat4("u_view",  view_);
  simulate_shader_->setMat4("u_proj",  proj_);

  // Bind scene depth texture to texture unit 0 for depth-collision sampling
  if (scene_depth_texture_ != 0) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_depth_texture_);
    simulate_shader_->setInt("u_sceneDepth", 0);
  }

  for (Entity entity : emitters) {
    if (!registry.hasComponent<ECS::Components::ParticlePoolComponent>(entity) ||
        !registry.hasComponent<ECS::Components::ParticleEmitter>(entity)) continue;
    auto &pool    = registry.getComponent<ECS::Components::ParticlePoolComponent>(entity);
    auto &emitter = registry.getComponent<ECS::Components::ParticleEmitter>(entity);
    if (pool.active_count == 0) continue;

    simulate_shader_->setUInt("u_emitterIndex",    pool.emitter_index);
    simulate_shader_->setUInt("u_poolOffset",      pool.pool_offset);
    simulate_shader_->setFloat("u_bounciness",     emitter.bounciness);
    simulate_shader_->setFloat("u_friction",       emitter.friction);
    simulate_shader_->setFloat("u_turbulence",     emitter.turbulence);
    simulate_shader_->setFloat("u_floorHeight",    emitter.floorHeight);
    simulate_shader_->setInt("u_collisionEnabled", emitter.collisionEnabled ? 1 : 0);

    const uint32_t groups = (pool.active_count + 63) / 64;
    simulate_shader_->dispatch(groups);
  }

  // Global memory barrier for SSBOs (wait for simulation / kill-list generation)
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // ── 2b. Sub-Emitter Phase ───────────────────────────────────────────────────
  // Bind sub-emitter SSBOs (bindings 16, 17)
  gpuBuf.bindSubEmitterSsbos();

  for (Entity entity : emitters) {
    if (!registry.hasComponent<ECS::Components::ParticleEmitter>(entity) ||
        !registry.hasComponent<ECS::Components::ParticlePoolComponent>(entity)) continue;

    auto &emitter = registry.getComponent<ECS::Components::ParticleEmitter>(entity);
    auto &pool    = registry.getComponent<ECS::Components::ParticlePoolComponent>(entity);

    if (!emitter.subEmitterEnabled || pool.active_count == 0 ||
        emitter.childEmitterEntity == std::numeric_limits<uint32_t>::max()) continue;

    // We have a valid sub-emitter. Get child.
    Entity childEntity = static_cast<Entity>(emitter.childEmitterEntity);
    if (!registry.hasComponent<ECS::Components::ParticleEmitter>(childEntity) ||
        !registry.hasComponent<ECS::Components::ParticlePoolComponent>(childEntity)) continue;

    auto &childEmitter = registry.getComponent<ECS::Components::ParticleEmitter>(childEntity);
    auto &childPool    = registry.getComponent<ECS::Components::ParticlePoolComponent>(childEntity);

    // Reset event counter
    gpuBuf.resetSpawnEvents();

    // 1) Gather dying particles into SpawnEvents array
    emit_events_shader_->bind();
    emit_events_shader_->setUInt("u_emitterIndex", pool.emitter_index);
    emit_events_shader_->setUInt("u_poolOffset",   pool.pool_offset);
    emit_events_shader_->setUInt("u_maxEvents",    gpuBuf.getMaxTotalParticles());
    // Safe dispatch using pool.active_count (which is >= killCount)
    emit_events_shader_->dispatch((pool.active_count + 63) / 64);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 2) Spawn children from Gathered Events
    spawn_from_events_shader_->bind();
    spawn_from_events_shader_->setUInt("u_childEmitterIndex",  childPool.emitter_index);
    spawn_from_events_shader_->setUInt("u_childPoolOffset",    childPool.pool_offset);
    spawn_from_events_shader_->setUInt("u_childMaxParticles",  childPool.pool_size);
    spawn_from_events_shader_->setUInt("u_spawnCountPerEvent", emitter.spawnCountOnDeath);
    spawn_from_events_shader_->setFloat("u_childSpeedScale",   emitter.childSpeedScale);
    spawn_from_events_shader_->setFloat("u_childLifetime",     childEmitter.particleLifetime);
    spawn_from_events_shader_->setVec4("u_childStartColor",    childEmitter.startColor);
    spawn_from_events_shader_->setVec4("u_childEndColor",      childEmitter.endColor);
    
    // Exact event count is required so we use the lagging active_count or kill limit as an upper-bound.
    // However, since this runs right after simulation, CPU doesn't know the exact count without stalling.
    // Instead we can safely dispatch exactly enough groups to cover max theoretically possible events (which is active_count).
    spawn_from_events_shader_->dispatch((pool.active_count + 63) / 64);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }

  // 3. Compact Phase
  compact_shader_->bind();
  for (Entity entity : emitters) {
    if (!registry.hasComponent<ECS::Components::ParticlePoolComponent>(entity)) continue;
    auto &pool = registry.getComponent<ECS::Components::ParticlePoolComponent>(entity);
    if (pool.active_count == 0 && pool.pool_size > 0) continue; 
    
    compact_shader_->setUInt("u_emitterIndex", pool.emitter_index);
    compact_shader_->setUInt("u_poolOffset", pool.pool_offset);
    compact_shader_->dispatch(1);
  }
  compact_shader_->unbind();

  // Final barrier for MultiDraw AND CPU Reads
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

  // 4. Update CPU active_count stats (lagged 1 frame)
  for (Entity entity : emitters) {
    if (!registry.hasComponent<ECS::Components::ParticlePoolComponent>(entity) ||
        !registry.hasComponent<ECS::Components::ParticleEmitter>(entity))
      continue;
    
    auto &pool = registry.getComponent<ECS::Components::ParticlePoolComponent>(entity);
    auto &emitter = registry.getComponent<ECS::Components::ParticleEmitter>(entity);
    if (pool.pool_size > 0) {
      pool.active_count = gpuBuf.readActiveCount(pool.emitter_index);
      emitter.activeParticles = pool.active_count;
    }
  }

  elapsed_time_ += dt;

  auto end_time = std::chrono::high_resolution_clock::now();
  cpu_simulate_time_ms_ = std::chrono::duration<float, std::milli>(end_time - start_time).count();
}

} // namespace ParticleGL::ECS::Systems

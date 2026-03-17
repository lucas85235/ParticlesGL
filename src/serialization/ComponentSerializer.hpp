#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

#include "../ecs/components/Lifetime.hpp"
#include "../ecs/components/ParticleEmitter.hpp"
#include "../ecs/components/Transform.hpp"

namespace glm {
inline void to_json(nlohmann::json &j, const glm::vec3 &v) {
  j = nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

inline void from_json(const nlohmann::json &j, glm::vec3 &v) {
  j.at("x").get_to(v.x);
  j.at("y").get_to(v.y);
  j.at("z").get_to(v.z);
}

inline void to_json(nlohmann::json &j, const glm::vec4 &v) {
  j = nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}, {"w", v.w}};
}

inline void from_json(const nlohmann::json &j, glm::vec4 &v) {
  j.at("x").get_to(v.x);
  j.at("y").get_to(v.y);
  j.at("z").get_to(v.z);
  j.at("w").get_to(v.w);
}

inline void to_json(nlohmann::json &j, const glm::quat &q) {
  j = nlohmann::json{{"x", q.x}, {"y", q.y}, {"z", q.z}, {"w", q.w}};
}

inline void from_json(const nlohmann::json &j, glm::quat &q) {
  j.at("x").get_to(q.x);
  j.at("y").get_to(q.y);
  j.at("z").get_to(q.z);
  j.at("w").get_to(q.w);
}
} // namespace glm

namespace ParticleGL::ECS::Components {

inline void to_json(nlohmann::json &j, const Transform &t) {
  j = nlohmann::json{
      {"position", t.position}, {"rotation", t.rotation}, {"scale", t.scale}};
}

inline void from_json(const nlohmann::json &j, Transform &t) {
  j.at("position").get_to(t.position);
  j.at("rotation").get_to(t.rotation);
  j.at("scale").get_to(t.scale);
}

inline void to_json(nlohmann::json &j, const ParticleEmitter &pe) {
  j = nlohmann::json{{"emissionRate", pe.emissionRate},
                     {"timeSinceLastEmission", pe.timeSinceLastEmission},
                     {"initialVelocity", pe.initialVelocity},
                     {"spreadAngle", pe.spreadAngle},
                     {"startColor", pe.startColor},
                     {"endColor", pe.endColor},
                     {"particleLifetime", pe.particleLifetime},
                     {"maxParticles", pe.maxParticles},
                     {"activeParticles", pe.activeParticles}};
}

inline void from_json(const nlohmann::json &j, ParticleEmitter &pe) {
  j.at("emissionRate").get_to(pe.emissionRate);
  j.at("timeSinceLastEmission").get_to(pe.timeSinceLastEmission);
  j.at("initialVelocity").get_to(pe.initialVelocity);
  j.at("spreadAngle").get_to(pe.spreadAngle);
  j.at("startColor").get_to(pe.startColor);
  j.at("endColor").get_to(pe.endColor);
  j.at("particleLifetime").get_to(pe.particleLifetime);
  j.at("maxParticles").get_to(pe.maxParticles);
  j.at("activeParticles").get_to(pe.activeParticles);
}

inline void to_json(nlohmann::json &j, const Lifetime &l) {
  j = nlohmann::json{
      {"current", l.current}, {"max", l.max}, {"active", l.active}};
}

inline void from_json(const nlohmann::json &j, Lifetime &l) {
  j.at("current").get_to(l.current);
  j.at("max").get_to(l.max);
  j.at("active").get_to(l.active);
}

} // namespace ParticleGL::ECS::Components

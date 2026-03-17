#pragma once

#include "../ecs/Registry.hpp"
#include <string>

namespace ParticleGL::Serialization {

class SceneSerializer {
public:
  SceneSerializer(ECS::Registry *registry) : registry_(registry) {}

  void serialize(const std::string &filepath);
  bool deserialize(const std::string &filepath);

private:
  ECS::Registry *registry_;
};

} // namespace ParticleGL::Serialization

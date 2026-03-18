#pragma once

#include "core/Window.hpp"
#include "demo/ComputeExample.hpp"

#include <memory>

namespace ParticleGL::Demo {

class ComputeApp {
public:
  ComputeApp();
  ~ComputeApp();

  void run();

private:
  std::unique_ptr<Window> window_;
  ComputeExample computeExample_;
  bool running_ = true;
};

} // namespace ParticleGL::Demo

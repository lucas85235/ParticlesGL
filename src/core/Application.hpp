#pragma once

#include "Window.hpp"

#include <memory>

namespace ParticleGL {

class Application {
public:
  Application();
  virtual ~Application();

  void run();
  void close();

  inline static Application &get() { return *s_instance_; }
  inline Window &getWindow() { return *window_; }

private:
  std::unique_ptr<Window> window_;
  bool running_ = true;

  static Application *s_instance_;
};

} // namespace ParticleGL

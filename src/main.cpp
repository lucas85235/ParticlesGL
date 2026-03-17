#include "core/Application.hpp"
#include "core/Logger.hpp"

#include <exception>

int main() {
  try {
    ParticleGL::Application app;
    PGL_INFO("Application created successfully");
    app.run();
  } catch (const std::exception &e) {
    PGL_ERROR("Fatal exception: " << e.what());
    return -1;
  }
  return 0;
}

#include "core/Logger.hpp"
#include "demo/ComputeApp.hpp"

#include <exception>

int main() {
  try {
    ParticleGL::Demo::ComputeApp app;
    app.run();
  } catch (const std::exception &e) {
    PGL_ERROR("Fatal exception in Compute Demo: " << e.what());
    return -1;
  }
  return 0;
}

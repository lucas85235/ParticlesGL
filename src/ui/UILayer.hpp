#pragma once

struct GLFWwindow;

namespace ParticleGL::UI {

class UILayer {
public:
  UILayer() = default;
  ~UILayer() = default;

  UILayer(const UILayer &) = delete;
  UILayer &operator=(const UILayer &) = delete;
  UILayer(UILayer &&) = delete;
  UILayer &operator=(UILayer &&) = delete;

  void init(GLFWwindow *window);
  void shutdown();

  void beginFrame();
  void endFrame();

private:
  void setDarkThemeColors();
};

} // namespace ParticleGL::UI

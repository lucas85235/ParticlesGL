#pragma once
#include <functional>
#include <string>

struct GLFWwindow;

namespace ParticleGL {

struct WindowProps {
  std::string title = "ParticleGL";
  uint32_t width = 1280;
  uint32_t height = 720;
};

class Window {
public:
  Window(const WindowProps &props = WindowProps());
  ~Window();

  void onUpdate();
  bool shouldClose() const;

  using EventCallbackFn = std::function<void(uint32_t, uint32_t)>;

  inline uint32_t getWidth() const { return data_.width; }
  inline uint32_t getHeight() const { return data_.height; }
  inline GLFWwindow *getNativeWindow() const { return window_; }

  void setEventCallback(const EventCallbackFn &callback) {
    data_.eventCallback = callback;
  }

private:
  void init(const WindowProps &props);
  void shutdown();

  GLFWwindow *window_;

  struct WindowData {
    std::string title;
    uint32_t width;
    uint32_t height;
    EventCallbackFn eventCallback;
  };

  WindowData data_;
};

} // namespace ParticleGL

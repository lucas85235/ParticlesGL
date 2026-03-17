#include "Window.hpp"

#include "Logger.hpp"

#include <glad/glad.h>

#include <GLFW/glfw3.h>

namespace ParticleGL {

static bool s_glfw_initialized = false;

static void glfwErrorCallback(int error, const char *description) {
  PGL_ERROR("GLFW Error (" << error << "): " << description);
}

Window::Window(const WindowProps &props) { init(props); }

Window::~Window() { shutdown(); }

void Window::init(const WindowProps &props) {
  data_.title = props.title;
  data_.width = props.width;
  data_.height = props.height;

  PGL_INFO("Creating window " << props.title << " (" << props.width << ", "
                              << props.height << ")");

  if (!s_glfw_initialized) {
    int success = glfwInit();
    if (!success) {
      PGL_ERROR("Could not initialize GLFW!");
      return;
    }
    glfwSetErrorCallback(glfwErrorCallback);
    s_glfw_initialized = true;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window_ = glfwCreateWindow((int)props.width, (int)props.height,
                             data_.title.c_str(), nullptr, nullptr);
  if (!window_) {
    PGL_ERROR("Failed to create GLFW window!");
    return;
  }

  glfwMakeContextCurrent(window_);
  glfwSetWindowUserPointer(window_, &data_);

  glfwSetFramebufferSizeCallback(
      window_, [](GLFWwindow *window, int width, int height) {
        WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
        data.width = width;
        data.height = height;
        if (data.eventCallback) {
          data.eventCallback(width, height);
        }
      });

  // Initialize GLAD
  int version = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  if (version == 0) {
    PGL_ERROR("Failed to initialize GLAD");
    return;
  }

  PGL_INFO("OpenGL Info:");
  PGL_INFO("  Vendor: " << glGetString(GL_VENDOR));
  PGL_INFO("  Renderer: " << glGetString(GL_RENDERER));
  PGL_INFO("  Version: " << glGetString(GL_VERSION));

  glfwSwapInterval(1); // V-Sync
}

void Window::shutdown() {
  glfwDestroyWindow(window_);
  if (s_glfw_initialized) {
    glfwTerminate();
    s_glfw_initialized = false;
  }
}

void Window::onUpdate() {
  glfwPollEvents();
  glfwSwapBuffers(window_);
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_); }

} // namespace ParticleGL

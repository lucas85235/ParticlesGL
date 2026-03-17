#pragma once

#include <iostream>

#define PGL_INFO(...)                                                          \
  {                                                                            \
    std::cout << "[INFO] " << __VA_ARGS__ << std::endl;                        \
  }
#define PGL_ERROR(...)                                                         \
  {                                                                            \
    std::cerr << "[ERROR] " << __VA_ARGS__ << std::endl;                       \
  }

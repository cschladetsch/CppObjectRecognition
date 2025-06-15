#pragma once

#include <algorithm>

// Helper function to clamp a value between min and max
template <typename T> inline T Clamp(T value, T min, T max) {
  return std::max(min, std::min(value, max));
}
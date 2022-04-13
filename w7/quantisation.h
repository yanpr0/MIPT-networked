#pragma once
#include "mathUtils.h"
#include <limits>

template<typename T>
T pack_float(float v, float lo, float hi)
{
  T range = std::numeric_limits<T>::max();
  return range * ((clamp(v, lo, hi) - lo) / (hi - lo));
}

template<typename T>
float unpack_float(T c, float lo, float hi)
{
  T range = std::numeric_limits<T>::max();
  return float(c) / range * (hi - lo) + lo;
}


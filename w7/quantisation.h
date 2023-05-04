#pragma once
#include "mathUtils.h"
#include <array>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>

template<typename T>
T pack_float(float v, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
  return range * ((clamp(v, lo, hi) - lo) / (hi - lo));
}

template<typename T>
float unpack_float(T c, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
  return float(c) / range * (hi - lo) + lo;
}

template<typename T, int num_bits>
struct PackedFloat
{
  T packedVal;

  PackedFloat(float v, float lo, float hi) { pack(v, lo, hi); }
  PackedFloat(T compressed_val) : packedVal(compressed_val) {}

  void pack(float v, float lo, float hi) { packedVal = pack_float<T>(v, lo, hi, num_bits); }
  float unpack(float lo, float hi) { return unpack_float<T>(packedVal, lo, hi, num_bits); }
};

typedef PackedFloat<uint8_t, 4> float4bitsQuantized;

template <std::unsigned_integral T, std::size_t num_bits1, std::size_t num_bits2>
requires
(
  num_bits1 > 0 &&
  num_bits2 > 0 &&
  sizeof(T) * CHAR_BIT >= num_bits1 &&
  sizeof(T) * CHAR_BIT >= num_bits2 &&
  sizeof(T) * CHAR_BIT >= num_bits1 + num_bits2
)
struct PackedVec2
{
  T packedVal;

  PackedVec2(std::array<float, 2> v, std::array<float, 2> lo, std::array<float, 2> hi) { pack(v, lo, hi); }
  PackedVec2(T compressed_val) : packedVal(compressed_val) {}

  void pack(std::array<float, 2> v, std::array<float, 2> lo, std::array<float, 2> hi)
  {
    packedVal =
      (pack_float<T>(v[0], lo[0], hi[0], num_bits1) << 0) |
      (pack_float<T>(v[1], lo[1], hi[1], num_bits2) << num_bits1);
  }

  std::array<float, 2> unpack(std::array<float, 2> lo, std::array<float, 2> hi)
  {
    T packedX = packedVal & ((static_cast<T>(1) << num_bits1) - 1);
    T packedY = packedVal >> num_bits1;

    return
    {
      unpack_float<T>(packedX, lo[0], hi[0], num_bits1),
      unpack_float<T>(packedY, lo[1], hi[1], num_bits2)
    };
  }
};

template <std::unsigned_integral T, std::size_t num_bits1, std::size_t num_bits2, std::size_t num_bits3>
requires
(
  num_bits1 > 0 &&
  num_bits2 > 0 &&
  num_bits3 > 0 &&
  sizeof(T) * CHAR_BIT >= num_bits1 &&
  sizeof(T) * CHAR_BIT >= num_bits2 &&
  sizeof(T) * CHAR_BIT >= num_bits3 &&
  sizeof(T) * CHAR_BIT >= num_bits1 + num_bits2 + num_bits3
)
struct PackedVec3
{
  T packedVal;

  PackedVec3(std::array<float, 3> v, std::array<float, 3> lo, std::array<float, 3> hi) { pack(v, lo, hi); }
  PackedVec3(T compressed_val) : packedVal(compressed_val) {}

  void pack(std::array<float, 3> v, std::array<float, 3> lo, std::array<float, 3> hi)
  {
    packedVal =
      (pack_float<T>(v[0], lo[0], hi[0], num_bits1) << 0) |
      (pack_float<T>(v[1], lo[1], hi[1], num_bits2) << num_bits1) |
      (pack_float<T>(v[2], lo[2], hi[2], num_bits3) << (num_bits1 + num_bits2));
  }

  std::array<float, 3> unpack(std::array<float, 3> lo, std::array<float, 3> hi)
  {
    T packedX = packedVal & ((static_cast<T>(1) << num_bits1) - 1);
    T packedY = (packedVal >> num_bits1) & ((static_cast<T>(1) << num_bits2) - 1);
    T packedZ = (packedVal >> (num_bits1 + num_bits2)) & ((static_cast<T>(1) << num_bits3) - 1);

    return
    {
      unpack_float<T>(packedX, lo[0], hi[0], num_bits1),
      unpack_float<T>(packedY, lo[1], hi[1], num_bits2),
      unpack_float<T>(packedZ, lo[2], hi[2], num_bits3)
    };
  }
};

std::uint32_t read_packed_uint32(std::istream& is)
{
  std::uint32_t res = 0;

  std::uint8_t byte = 0;
  std::size_t offset = 0;
  while (is >> byte)
  {
    res |= static_cast<std::uint32_t>(byte & 0x7F) << offset;
    if (byte & 0x80)
    {
        offset += 7;
    }
    else
    {
        return res;
    }
  }
  return -1; // error
}

void write_packed_uint32(std::uint32_t val, std::ostream& os)
{
  if (val == 0)
  {
    os << '\0';
    return;
  }

  while (val > 0)
  {
    if (val > 0x7F)
    {
      os << static_cast<std::uint8_t>((val & 0x7F) | 0x80);
      val >>= 7;
    }
    else
    {
      os << static_cast<std::uint8_t>(val);
      return;
    }
  }
}

#ifndef POLYSEG_SEGCORE_TYPES_H
#define POLYSEG_SEGCORE_TYPES_H

#include <cstdint>

namespace segcore {

struct Point2D
{
  int x = 0;
  int y = 0;

  bool operator==(const Point2D& other) const
  {
    return x == other.x && y == other.y;
  }
};

struct Rgb24
{
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  bool operator==(const Rgb24& other) const
  {
    return r == other.r && g == other.g && b == other.b;
  }
};

using ArtifactId = uint32_t;
using SegmentId = uint32_t;

constexpr ArtifactId kInvalidArtifactId = 0;
constexpr SegmentId kInvalidSegmentId = 0;

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_TYPES_H

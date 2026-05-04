#ifndef POLYSEG_SEGCORE_SEGMENT_H
#define POLYSEG_SEGCORE_SEGMENT_H

#include <vector>
#include <segcore/types.h>

namespace segcore {

struct Segment
{
  SegmentId id = kInvalidSegmentId;
  int class_id = 0;
  bool selected = false;
  std::vector<Point2D> points;
};

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_SEGMENT_H

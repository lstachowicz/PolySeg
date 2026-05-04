#ifndef POLYSEG_SEGCORE_GEOMETRY_H
#define POLYSEG_SEGCORE_GEOMETRY_H

#include <segcore/segment.h>
#include <segcore/types.h>

#include <vector>

namespace segcore
{

struct PointHit
{
  SegmentId segment_id = kInvalidSegmentId;
  int point_index = -1;
};

PointHit HitTestPoint(const std::vector<Segment>& segments, Point2D pos, int tolerance);
SegmentId HitTestSegment(const std::vector<Segment>& segments, Point2D pos);
int InsertionIndex(const Segment& seg, Point2D pos);
bool PointInPolygon(const std::vector<Point2D>& pts, Point2D pos);

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_GEOMETRY_H

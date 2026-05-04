#include <segcore/geometry.h>

#include <cmath>
#include <limits>

namespace segcore {

namespace {

double Distance(Point2D a, Point2D b)
{
  double dx = a.x - b.x;
  double dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

double DistanceFromPointToSegment(Point2D p, Point2D a, Point2D b)
{
  double dx = b.x - a.x;
  double dy = b.y - a.y;
  double len_sq = dx * dx + dy * dy;
  if (len_sq < 1e-10)
  {
    return Distance(p, a);
  }
  double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len_sq;
  t = std::max(0.0, std::min(1.0, t));
  Point2D proj{static_cast<int>(a.x + t * dx), static_cast<int>(a.y + t * dy)};
  return Distance(p, proj);
}

}  // namespace

PointHit HitTestPoint(const std::vector<Segment>& segments, Point2D pos, int tolerance)
{
  for (const auto& seg : segments)
  {
    for (int i = 0; i < static_cast<int>(seg.points.size()); ++i)
    {
      if (Distance(seg.points[i], pos) <= static_cast<double>(tolerance))
      {
        return {seg.id, i};
      }
    }
  }
  return {kInvalidSegmentId, -1};
}

SegmentId HitTestSegment(const std::vector<Segment>& segments, Point2D pos)
{
  for (const auto& seg : segments)
  {
    if (PointInPolygon(seg.points, pos))
    {
      return seg.id;
    }
  }
  return kInvalidSegmentId;
}

int InsertionIndex(const Segment& seg, Point2D pos)
{
  if (seg.points.size() < 2)
  {
    return static_cast<int>(seg.points.size());
  }
  int best_index = 1;
  double best_dist = std::numeric_limits<double>::max();
  int n = static_cast<int>(seg.points.size());
  for (int i = 0; i < n; ++i)
  {
    int j = (i + 1) % n;
    double d = DistanceFromPointToSegment(pos, seg.points[i], seg.points[j]);
    if (d < best_dist)
    {
      best_dist = d;
      best_index = j == 0 ? n : j;
    }
  }
  return best_index;
}

bool PointInPolygon(const std::vector<Point2D>& pts, Point2D pos)
{
  int n = static_cast<int>(pts.size());
  if (n < 3)
  {
    return false;
  }
  bool inside = false;
  for (int i = 0, j = n - 1; i < n; j = i++)
  {
    int xi = pts[i].x;
    int yi = pts[i].y;
    int xj = pts[j].x;
    int yj = pts[j].y;
    bool intersects = ((yi > pos.y) != (yj > pos.y)) &&
                      (pos.x < (xj - xi) * (pos.y - yi) / (yj - yi) + xi);
    if (intersects)
    {
      inside = !inside;
    }
  }
  return inside;
}

}  // namespace segcore

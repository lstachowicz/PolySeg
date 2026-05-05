#include <gtest/gtest.h>
#include <segcore/geometry.h>

using namespace segcore;

static Segment MakeSquare(SegmentId id)
{
  Segment s;
  s.id = id;
  s.points = {{0, 0}, {100, 0}, {100, 100}, {0, 100}};
  return s;
}

TEST(PointInPolygonTest, InsideSquare)
{
  auto square = MakeSquare(1);
  EXPECT_TRUE(PointInPolygon(square.points, {50, 50}));
}

TEST(PointInPolygonTest, OutsideSquare)
{
  auto square = MakeSquare(1);
  EXPECT_FALSE(PointInPolygon(square.points, {200, 200}));
}

TEST(PointInPolygonTest, TooFewPoints)
{
  std::vector<Point2D> pts = {{0, 0}, {10, 0}};
  EXPECT_FALSE(PointInPolygon(pts, {5, 0}));
}

TEST(HitTestPointTest, HitsVertex)
{
  std::vector<Segment> segs = {MakeSquare(1)};
  PointHit hit = HitTestPoint(segs, {2, 2}, 5);
  EXPECT_EQ(hit.segment_id, 1u);
  EXPECT_EQ(hit.point_index, 0);
}

TEST(HitTestPointTest, MissesVertex)
{
  std::vector<Segment> segs = {MakeSquare(1)};
  PointHit hit = HitTestPoint(segs, {50, 50}, 5);
  EXPECT_EQ(hit.segment_id, kInvalidSegmentId);
  EXPECT_EQ(hit.point_index, -1);
}

TEST(HitTestSegmentTest, InsidePolygon)
{
  std::vector<Segment> segs = {MakeSquare(1)};
  EXPECT_EQ(HitTestSegment(segs, {50, 50}), 1u);
}

TEST(HitTestSegmentTest, OutsidePolygon)
{
  std::vector<Segment> segs = {MakeSquare(1)};
  EXPECT_EQ(HitTestSegment(segs, {200, 200}), kInvalidSegmentId);
}

TEST(InsertionIndexTest, ReturnsValidIndex)
{
  Segment s = MakeSquare(1);
  int idx = InsertionIndex(s, {50, 0});
  EXPECT_GE(idx, 0);
  EXPECT_LE(idx, static_cast<int>(s.points.size()));
}

TEST(InsertionIndexTest, TopEdgeInsertsAt1)
{
  Segment s = MakeSquare(1);
  // Point on top edge (between points[0]={0,0} and points[1]={100,0})
  int idx = InsertionIndex(s, {50, 0});
  EXPECT_EQ(idx, 1);
}

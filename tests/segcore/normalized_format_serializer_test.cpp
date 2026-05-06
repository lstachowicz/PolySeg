#include <gtest/gtest.h>
#include <segcore/normalized_format_serializer.h>

using namespace segcore;

static Segment MakeTriangle(SegmentId id, int class_id = 0)
{
  Segment s;
  s.id = id;
  s.class_id = class_id;
  s.points = {{100, 200}, {300, 400}, {500, 600}};
  return s;
}

TEST(YoloSerializerTest, SerializeSingleSegment)
{
  std::vector<Segment> segs = {MakeTriangle(1)};
  std::string out = SegmentsToNormalizedFormat(segs, 1000, 1000);
  EXPECT_EQ(out, "0 0.100000 0.200000 0.300000 0.400000 0.500000 0.600000\n");
}

TEST(YoloSerializerTest, SerializeMultipleSegments)
{
  Segment s1 = MakeTriangle(1, 0);
  Segment s2 = MakeTriangle(2, 1);
  std::string out = SegmentsToNormalizedFormat({s1, s2}, 1000, 1000);
  EXPECT_EQ(out,
            "0 0.100000 0.200000 0.300000 0.400000 0.500000 0.600000\n"
            "1 0.100000 0.200000 0.300000 0.400000 0.500000 0.600000\n");
}

TEST(YoloSerializerTest, SkipsSegmentWithFewPoints)
{
  Segment s;
  s.id = 1;
  s.points = {{10, 20}, {30, 40}};
  std::string out = SegmentsToNormalizedFormat({s}, 1000, 1000);
  EXPECT_TRUE(out.empty());
}

TEST(YoloDeserializerTest, DeserializeValidLine)
{
  std::string text = "0 0.100 0.200 0.300 0.400 0.500 0.600\n";
  auto segs = NormalizedFormatToSegments(text, 1000, 1000);
  ASSERT_EQ(segs.size(), 1u);
  EXPECT_EQ(segs[0].class_id, 0);
  ASSERT_EQ(segs[0].points.size(), 3u);
  EXPECT_EQ(segs[0].points[0].x, 100);
  EXPECT_EQ(segs[0].points[0].y, 200);
}

TEST(YoloDeserializerTest, SkipsLineTooShort)
{
  std::string text = "0 0.1 0.2 0.3 0.4\n";
  auto segs = NormalizedFormatToSegments(text, 1000, 1000);
  EXPECT_TRUE(segs.empty());
}

TEST(YoloSerializerTest, RoundTrip)
{
  std::vector<Segment> original = {MakeTriangle(1, 2)};
  int w = 1000;
  int h = 1000;
  std::string serialized = SegmentsToNormalizedFormat(original, w, h);
  auto deserialized = NormalizedFormatToSegments(serialized, w, h);
  ASSERT_EQ(deserialized.size(), 1u);
  EXPECT_EQ(deserialized[0].class_id, original[0].class_id);
  ASSERT_EQ(deserialized[0].points.size(), original[0].points.size());
  for (size_t i = 0; i < original[0].points.size(); ++i)
  {
    EXPECT_EQ(deserialized[0].points[i].x, original[0].points[i].x);
    EXPECT_EQ(deserialized[0].points[i].y, original[0].points[i].y);
  }
}

// Coordinates that are not evenly divisible by image dimensions.
// With 3 decimal places + truncation these would drift by 1px per save/load cycle:
//   105/640 = 0.164062... -> "0.164" -> int(104.96) = 104  (drift)
//   100/480 = 0.208333... -> "0.208" -> int(99.84)  = 99   (drift)
//   160/480 = 0.333333... -> "0.333" -> int(159.84) = 159  (drift)
TEST(YoloSerializerTest, RoundTripNonDivisibleCoordinates)
{
  Segment s;
  s.id = 1;
  s.class_id = 0;
  s.points = {{105, 100}, {171, 160}, {320, 399}};

  const int w = 640;
  const int h = 480;

  std::string serialized = SegmentsToNormalizedFormat({s}, w, h);
  auto deserialized = NormalizedFormatToSegments(serialized, w, h);

  ASSERT_EQ(deserialized.size(), 1u);
  ASSERT_EQ(deserialized[0].points.size(), s.points.size());
  for (size_t i = 0; i < s.points.size(); ++i)
  {
    EXPECT_EQ(deserialized[0].points[i].x, s.points[i].x);
    EXPECT_EQ(deserialized[0].points[i].y, s.points[i].y);
  }
}

// Simulates navigating away and back to the same image multiple times.
// Each cycle: serialize current points, then deserialize back.
// Points must be identical after every cycle - no accumulated drift.
TEST(YoloSerializerTest, RoundTripStableAcrossMultipleCycles)
{
  Segment s;
  s.id = 1;
  s.class_id = 0;
  s.points = {{105, 100}, {171, 160}, {320, 399}};

  const int w = 640;
  const int h = 480;

  std::string text = SegmentsToNormalizedFormat({s}, w, h);
  auto after_first = NormalizedFormatToSegments(text, w, h);
  ASSERT_EQ(after_first.size(), 1u);

  for (int cycle = 0; cycle < 5; ++cycle)
  {
    text = SegmentsToNormalizedFormat(after_first, w, h);
    auto after_cycle = NormalizedFormatToSegments(text, w, h);
    ASSERT_EQ(after_cycle.size(), 1u);
    ASSERT_EQ(after_cycle[0].points.size(), after_first[0].points.size());
    for (size_t i = 0; i < after_first[0].points.size(); ++i)
    {
      EXPECT_EQ(after_cycle[0].points[i].x, after_first[0].points[i].x)
          << "x drift at cycle " << cycle + 1 << ", point " << i;
      EXPECT_EQ(after_cycle[0].points[i].y, after_first[0].points[i].y)
          << "y drift at cycle " << cycle + 1 << ", point " << i;
    }
  }
}

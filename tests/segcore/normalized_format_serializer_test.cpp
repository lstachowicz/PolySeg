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
  EXPECT_EQ(out, "0 0.100 0.200 0.300 0.400 0.500 0.600\n");
}

TEST(YoloSerializerTest, SerializeMultipleSegments)
{
  Segment s1 = MakeTriangle(1, 0);
  Segment s2 = MakeTriangle(2, 1);
  std::string out = SegmentsToNormalizedFormat({s1, s2}, 1000, 1000);
  EXPECT_EQ(out,
            "0 0.100 0.200 0.300 0.400 0.500 0.600\n"
            "1 0.100 0.200 0.300 0.400 0.500 0.600\n");
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
    EXPECT_NEAR(deserialized[0].points[i].x, original[0].points[i].x, 1);
    EXPECT_NEAR(deserialized[0].points[i].y, original[0].points[i].y, 1);
  }
}

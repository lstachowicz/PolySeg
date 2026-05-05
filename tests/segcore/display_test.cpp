#include <gtest/gtest.h>
#include <segcore/display.h>

using namespace segcore;

TEST(DisplayTest, ImageArtifactPassthrough)
{
  Artifact a;
  ImageArtifact img;
  img.pixels = polyseg::Frame<Rgb24>(2, 3, Rgb24{10, 20, 30});
  a.payload = img;

  auto out = NormaliseForDisplay(a);
  EXPECT_EQ(out.width(), 2);
  EXPECT_EQ(out.height(), 3);
  EXPECT_EQ(out.data()[0].r, 10);
  EXPECT_EQ(out.data()[0].g, 20);
  EXPECT_EQ(out.data()[0].b, 30);
}

TEST(DisplayTest, FloatArtifactMidpoint)
{
  Artifact a;
  FloatArtifact fa;
  fa.data = polyseg::Frame<float>(1, 1, 0.5f);
  fa.range_min = 0.0f;
  fa.range_max = 1.0f;
  a.payload = fa;

  auto out = NormaliseForDisplay(a);
  EXPECT_EQ(out.width(), 1);
  EXPECT_EQ(out.height(), 1);
  // 0.5 * 255 = 127 (truncation)
  EXPECT_EQ(out.data()[0].r, 127);
  EXPECT_EQ(out.data()[0].g, 127);
  EXPECT_EQ(out.data()[0].b, 127);
}

TEST(DisplayTest, FloatArtifactBelowMin)
{
  Artifact a;
  FloatArtifact fa;
  fa.data = polyseg::Frame<float>(1, 1, -1.0f);
  fa.range_min = 0.0f;
  fa.range_max = 1.0f;
  a.payload = fa;

  auto out = NormaliseForDisplay(a);
  EXPECT_EQ(out.data()[0].r, 0);
}

TEST(DisplayTest, FloatArtifactAboveMax)
{
  Artifact a;
  FloatArtifact fa;
  fa.data = polyseg::Frame<float>(1, 1, 2.0f);
  fa.range_min = 0.0f;
  fa.range_max = 1.0f;
  a.payload = fa;

  auto out = NormaliseForDisplay(a);
  EXPECT_EQ(out.data()[0].r, 255);
}

TEST(DisplayTest, OutputDimensionsMatchInput)
{
  Artifact a;
  FloatArtifact fa;
  fa.data = polyseg::Frame<float>(10, 7, 0.5f);
  fa.range_min = 0.0f;
  fa.range_max = 1.0f;
  a.payload = fa;

  auto out = NormaliseForDisplay(a);
  EXPECT_EQ(out.width(), 10);
  EXPECT_EQ(out.height(), 7);
}

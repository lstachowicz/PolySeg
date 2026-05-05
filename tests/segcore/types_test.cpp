#include <gtest/gtest.h>
#include <segcore/artifact.h>
#include <segcore/types.h>

using namespace segcore;

TEST(TypesTest, InvalidIds)
{
  EXPECT_EQ(kInvalidArtifactId, 0u);
  EXPECT_EQ(kInvalidSegmentId, 0u);
}

TEST(ArtifactTest, ImageArtifactWidthHeight)
{
  Artifact a;
  ImageArtifact img;
  img.pixels = polyseg::Frame<Rgb24>(100, 80);
  a.payload = std::move(img);
  EXPECT_EQ(a.width(), 100);
  EXPECT_EQ(a.height(), 80);
}

TEST(ArtifactTest, FloatArtifactWidthHeight)
{
  Artifact a;
  FloatArtifact fa;
  fa.data = polyseg::Frame<float>(64, 48);
  a.payload = std::move(fa);
  EXPECT_EQ(a.width(), 64);
  EXPECT_EQ(a.height(), 48);
}

TEST(ArtifactTest, ImageArtifactPixelDataAccessible)
{
  Artifact a;
  ImageArtifact img;
  img.pixels = polyseg::Frame<Rgb24>(2, 2, Rgb24{1, 2, 3});
  a.payload = img;
  const auto& pixels = std::get<ImageArtifact>(a.payload).pixels;
  EXPECT_EQ(pixels.data()[0].r, 1);
  EXPECT_EQ(pixels.data()[0].g, 2);
  EXPECT_EQ(pixels.data()[0].b, 3);
}

TEST(ArtifactTest, FloatArtifactDataAccessible)
{
  Artifact a;
  FloatArtifact fa;
  fa.data = polyseg::Frame<float>(1, 1, 0.5f);
  fa.range_min = 0.0f;
  fa.range_max = 1.0f;
  a.payload = fa;
  const auto& data = std::get<FloatArtifact>(a.payload).data;
  EXPECT_FLOAT_EQ(data.data()[0], 0.5f);
}

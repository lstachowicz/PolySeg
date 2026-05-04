#include <gtest/gtest.h>
#include <segcore/project.h>

using namespace segcore;

class ProjectTest : public ::testing::Test
{
 protected:
  Project project_;

  ArtifactId AddImageArtifact()
  {
    Artifact a;
    ImageArtifact img;
    img.pixels = polyseg::Frame<Rgb24>(10, 10);
    img.source_path = "test.png";
    a.payload = std::move(img);
    return project_.AddArtifact(std::move(a));
  }
};

TEST_F(ProjectTest, AddArtifactReturnsValidId)
{
  ArtifactId id = AddImageArtifact();
  EXPECT_NE(id, kInvalidArtifactId);
}

TEST_F(ProjectTest, GetArtifactNonNull)
{
  ArtifactId id = AddImageArtifact();
  EXPECT_NE(project_.GetArtifact(id), nullptr);
}

TEST_F(ProjectTest, GetAnnotationsEmptyInitially)
{
  ArtifactId id = AddImageArtifact();
  EXPECT_TRUE(project_.GetAnnotations(id).GetSegments().empty());
}

TEST_F(ProjectTest, AnnotationsIndependentPerArtifact)
{
  ArtifactId id1 = AddImageArtifact();
  ArtifactId id2 = AddImageArtifact();
  auto& ann1 = project_.GetAnnotations(id1);
  SegmentId seg_id = ann1.BeginSegment(0);
  ann1.AddPoint(seg_id, {0, 0});
  ann1.AddPoint(seg_id, {10, 0});
  ann1.AddPoint(seg_id, {5, 10});
  ann1.CommitSegment(seg_id);
  EXPECT_EQ(project_.GetAnnotations(id1).GetSegments().size(), 1u);
  EXPECT_EQ(project_.GetAnnotations(id2).GetSegments().size(), 0u);
}

TEST_F(ProjectTest, SetAndGetCurrentArtifactId)
{
  ArtifactId id = AddImageArtifact();
  EXPECT_EQ(project_.GetCurrentArtifactId(), kInvalidArtifactId);
  project_.SetCurrentArtifact(id);
  EXPECT_EQ(project_.GetCurrentArtifactId(), id);
}

TEST_F(ProjectTest, RemoveArtifactMakesGetArtifactNull)
{
  ArtifactId id = AddImageArtifact();
  project_.RemoveArtifact(id);
  EXPECT_EQ(project_.GetArtifact(id), nullptr);
}

TEST_F(ProjectTest, RemoveCurrentArtifactResetsCurrentId)
{
  ArtifactId id = AddImageArtifact();
  project_.SetCurrentArtifact(id);
  project_.RemoveArtifact(id);
  EXPECT_EQ(project_.GetCurrentArtifactId(), kInvalidArtifactId);
}

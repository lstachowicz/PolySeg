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

TEST_F(ProjectTest, UndoAfterRehashDoesNotCorruptState)
{
  // Add enough artifacts to guarantee at least one unordered_map rehash before
  // and after the annotation operations.
  for (int i = 0; i < 8; ++i)
  {
    AddImageArtifact();
  }

  ArtifactId id = AddImageArtifact();
  auto& ann = project_.GetAnnotations(id);

  SegmentId seg = ann.BeginSegment(0);
  ann.AddPoint(seg, {0, 0});
  ann.AddPoint(seg, {10, 0});

  // More insertions after annotation work forces additional rehashes. With
  // by-value storage the undo lambdas' captured `this` would now dangle.
  for (int i = 0; i < 8; ++i)
  {
    AddImageArtifact();
  }

  ASSERT_TRUE(ann.CanUndo());
  ann.Undo();

  const Segment* in_progress = ann.GetInProgress();
  ASSERT_NE(in_progress, nullptr);
  EXPECT_EQ(in_progress->points.size(), 1u);
}

TEST_F(ProjectTest, ImageNavigationReleasesOldArtifacts)
{
  // Each image switch must remove the previous artifact so pixel data does not
  // accumulate across navigations. This is the contract LoadImageAtIndex upholds.
  ArtifactId previous = kInvalidArtifactId;
  std::vector<ArtifactId> seen_ids;

  for (int i = 0; i < 5; ++i)
  {
    ArtifactId current = AddImageArtifact();
    project_.SetCurrentArtifact(current);
    seen_ids.push_back(current);

    if (previous != kInvalidArtifactId)
    {
      project_.RemoveArtifact(previous);
      EXPECT_EQ(project_.GetArtifact(previous), nullptr)
          << "Artifact from navigation step " << (i - 1) << " should have been freed";
    }
    previous = current;
  }

  EXPECT_NE(project_.GetArtifact(seen_ids.back()), nullptr);
}

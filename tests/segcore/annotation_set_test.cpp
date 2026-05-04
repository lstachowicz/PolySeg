#include <gtest/gtest.h>
#include <segcore/annotation_set.h>

using namespace segcore;

class AnnotationSetTest : public ::testing::Test
{
 protected:
  AnnotationSet set_;
};

// --- Lifecycle ---

TEST_F(AnnotationSetTest, BeginSegmentReturnsValidId)
{
  SegmentId id = set_.BeginSegment(0);
  EXPECT_NE(id, kInvalidSegmentId);
}

TEST_F(AnnotationSetTest, AddPointGrowsInProgress)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {10, 20});
  set_.AddPoint(id, {30, 40});
  const Segment* ip = set_.GetInProgress();
  ASSERT_NE(ip, nullptr);
  EXPECT_EQ(ip->points.size(), 2u);
}

TEST_F(AnnotationSetTest, CommitSegmentWithEnoughPoints)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  EXPECT_TRUE(set_.CommitSegment(id));
  EXPECT_EQ(set_.GetSegments().size(), 1u);
  EXPECT_EQ(set_.GetInProgress(), nullptr);
}

TEST_F(AnnotationSetTest, CommitSegmentWithTooFewPoints)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  EXPECT_FALSE(set_.CommitSegment(id));
  EXPECT_TRUE(set_.GetSegments().empty());
}

TEST_F(AnnotationSetTest, DeleteSegmentRemovesIt)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  set_.DeleteSegment(id);
  EXPECT_TRUE(set_.GetSegments().empty());
}

TEST_F(AnnotationSetTest, ClearAllEmptiesEverything)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  set_.ClearAll();
  EXPECT_TRUE(set_.GetSegments().empty());
  EXPECT_EQ(set_.GetInProgress(), nullptr);
}

// --- Point editing ---

TEST_F(AnnotationSetTest, MovePointUpdatesPosition)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  set_.MovePoint(id, 0, {99, 99});
  const Segment* seg = set_.GetSegment(id);
  ASSERT_NE(seg, nullptr);
  EXPECT_EQ(seg->points[0].x, 99);
  EXPECT_EQ(seg->points[0].y, 99);
}

TEST_F(AnnotationSetTest, RemovePointShrinks)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.AddPoint(id, {5, 20});
  set_.CommitSegment(id);
  set_.RemovePoint(id, 0);
  const Segment* seg = set_.GetSegment(id);
  ASSERT_NE(seg, nullptr);
  EXPECT_EQ(seg->points.size(), 3u);
}

TEST_F(AnnotationSetTest, InsertPointInsertsAtIndex)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  set_.InsertPoint(id, 1, {99, 99});
  const Segment* seg = set_.GetSegment(id);
  ASSERT_NE(seg, nullptr);
  EXPECT_EQ(seg->points[1].x, 99);
  EXPECT_EQ(seg->points.size(), 4u);
}

// --- Selection ---

TEST_F(AnnotationSetTest, SelectSegment)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  set_.SelectSegment(id);
  EXPECT_EQ(set_.GetSelectedSegmentId(), id);
  const Segment* seg = set_.GetSegment(id);
  ASSERT_NE(seg, nullptr);
  EXPECT_TRUE(seg->selected);
}

TEST_F(AnnotationSetTest, DeselectAll)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  set_.SelectSegment(id);
  set_.DeselectAll();
  EXPECT_EQ(set_.GetSelectedSegmentId(), kInvalidSegmentId);
}

// --- Copy/Paste ---

TEST_F(AnnotationSetTest, HasClipboardFalseInitially)
{
  EXPECT_FALSE(set_.HasClipboard());
}

TEST_F(AnnotationSetTest, CopyAndPaste)
{
  SegmentId id = set_.BeginSegment(2);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  set_.CopySegment(id);
  EXPECT_TRUE(set_.HasClipboard());
  SegmentId pasted_id = set_.PasteSegment();
  EXPECT_NE(pasted_id, kInvalidSegmentId);
  EXPECT_NE(pasted_id, id);
  EXPECT_EQ(set_.GetSegments().size(), 2u);
  const Segment* pasted = set_.GetSegment(pasted_id);
  ASSERT_NE(pasted, nullptr);
  EXPECT_EQ(pasted->class_id, 2);
  EXPECT_EQ(pasted->points.size(), 3u);
}

// --- Undo/Redo ---

TEST_F(AnnotationSetTest, UndoAddPoint)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  EXPECT_TRUE(set_.CanUndo());
  set_.Undo();
  const Segment* ip = set_.GetInProgress();
  ASSERT_NE(ip, nullptr);
  EXPECT_EQ(ip->points.size(), 1u);
}

TEST_F(AnnotationSetTest, UndoCommitRestoresInProgress)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  EXPECT_EQ(set_.GetSegments().size(), 1u);
  set_.Undo();
  EXPECT_TRUE(set_.GetSegments().empty());
  EXPECT_NE(set_.GetInProgress(), nullptr);
}

TEST_F(AnnotationSetTest, UndoDelete)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  set_.DeleteSegment(id);
  EXPECT_TRUE(set_.GetSegments().empty());
  set_.Undo();
  EXPECT_EQ(set_.GetSegments().size(), 1u);
}

TEST_F(AnnotationSetTest, UndoThenRedo)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  set_.Undo();
  EXPECT_TRUE(set_.GetSegments().empty());
  set_.Redo();
  EXPECT_EQ(set_.GetSegments().size(), 1u);
}

TEST_F(AnnotationSetTest, PasteSegmentCanBeUndone)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  set_.CopySegment(id);
  set_.PasteSegment();
  EXPECT_EQ(set_.GetSegments().size(), 2u);
  set_.Undo();
  EXPECT_EQ(set_.GetSegments().size(), 1u);
}

TEST_F(AnnotationSetTest, GetSetClipboard)
{
  SegmentId id = set_.BeginSegment(1);
  set_.AddPoint(id, {5, 5});
  set_.AddPoint(id, {15, 5});
  set_.AddPoint(id, {10, 15});
  set_.CommitSegment(id);
  
  // Copy segment to clipboard
  set_.CopySegment(id);
  EXPECT_TRUE(set_.HasClipboard());
  
  // Get clipboard and verify it matches original
  const Segment* clipboard = set_.GetClipboard();
  ASSERT_NE(clipboard, nullptr);
  EXPECT_EQ(clipboard->class_id, 1);
  EXPECT_EQ(clipboard->points.size(), 3u);
  
  // Create new annotation set and transfer clipboard
  AnnotationSet other_set;
  other_set.SetClipboard(clipboard);
  EXPECT_TRUE(other_set.HasClipboard());
  
  // Paste should work in new set
  SegmentId pasted = other_set.PasteSegment();
  EXPECT_NE(pasted, kInvalidSegmentId);
  const Segment* pasted_seg = other_set.GetSegment(pasted);
  ASSERT_NE(pasted_seg, nullptr);
  EXPECT_EQ(pasted_seg->class_id, 1);
  EXPECT_EQ(pasted_seg->points.size(), 3u);
}

TEST_F(AnnotationSetTest, ClearClipboardWithSetNull)
{
  SegmentId id = set_.BeginSegment(0);
  set_.AddPoint(id, {0, 0});
  set_.AddPoint(id, {10, 0});
  set_.AddPoint(id, {5, 10});
  set_.CommitSegment(id);
  
  // Copy to clipboard
  set_.CopySegment(id);
  EXPECT_TRUE(set_.HasClipboard());
  
  // Clear clipboard by setting to nullptr
  set_.SetClipboard(nullptr);
  EXPECT_FALSE(set_.HasClipboard());
}

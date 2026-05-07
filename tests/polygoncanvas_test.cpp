#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QApplication>
#include <QSignalSpy>
#include <QTest>

#include "polygoncanvas.h"
#include <segcore/iannotation_set.h>
#include <segcore/segment.h>
#include <segcore/types.h>

// Suppress gmock warnings from -Wshadow and -Wold-style-cast in generated code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wold-style-cast"

class MockAnnotationSet : public segcore::IAnnotationSet
{
 public:
  MOCK_METHOD(segcore::SegmentId, BeginSegment, (int), (override));
  MOCK_METHOD(void, AddPoint, (segcore::SegmentId, segcore::Point2D), (override));
  MOCK_METHOD(void, InsertPoint, (segcore::SegmentId, int, segcore::Point2D), (override));
  MOCK_METHOD(void, MovePoint, (segcore::SegmentId, int, segcore::Point2D), (override));
  MOCK_METHOD(void, RemovePoint, (segcore::SegmentId, int), (override));
  MOCK_METHOD(bool, CommitSegment, (segcore::SegmentId), (override));
  MOCK_METHOD(void, DeleteSegment, (segcore::SegmentId), (override));
  MOCK_METHOD(void, ClearAll, (), (override));
  MOCK_METHOD(const segcore::Segment*, GetSegment, (segcore::SegmentId), (const, override));
  MOCK_METHOD(const std::vector<segcore::Segment>&, GetSegments, (), (const, override));
  MOCK_METHOD(const segcore::Segment*, GetInProgress, (), (const, override));
  MOCK_METHOD(void, SelectSegment, (segcore::SegmentId), (override));
  MOCK_METHOD(void, DeselectAll, (), (override));
  MOCK_METHOD(segcore::SegmentId, GetSelectedSegmentId, (), (const, override));
  MOCK_METHOD(void, CopySegment, (segcore::SegmentId), (override));
  MOCK_METHOD(segcore::SegmentId, PasteSegment, (), (override));
  MOCK_METHOD(bool, HasClipboard, (), (const, override));
  MOCK_METHOD(void, CancelInProgress, (), (override));
  MOCK_METHOD(void, Undo, (), (override));
  MOCK_METHOD(void, Redo, (), (override));
  MOCK_METHOD(bool, CanUndo, (), (const, override));
  MOCK_METHOD(bool, CanRedo, (), (const, override));
};

#pragma GCC diagnostic pop

class PolygonCanvasTest : public testing::Test
{
 protected:
  void SetUp() override
  {
    canvas_ = new PolygonCanvas(nullptr);
    canvas_->resize(200, 200);
    canvas_->show();

    ON_CALL(mock_, GetSegments()).WillByDefault(testing::ReturnRef(empty_segments_));
    ON_CALL(mock_, GetInProgress()).WillByDefault(testing::Return(nullptr));
    ON_CALL(mock_, GetSelectedSegmentId())
        .WillByDefault(testing::Return(segcore::kInvalidSegmentId));
    ON_CALL(mock_, CanUndo()).WillByDefault(testing::Return(false));
    ON_CALL(mock_, CanRedo()).WillByDefault(testing::Return(false));
    ON_CALL(mock_, HasClipboard()).WillByDefault(testing::Return(false));
  }

  void TearDown() override { delete canvas_; }

  PolygonCanvas* canvas_;
  testing::NiceMock<MockAnnotationSet> mock_;
  std::vector<segcore::Segment> empty_segments_;
};

TEST_F(PolygonCanvasTest, SetAnnotationSetDelegatesToMock)
{
  canvas_->SetAnnotationSet(&mock_);
  EXPECT_CALL(mock_, CanUndo()).WillOnce(testing::Return(true));
  EXPECT_TRUE(canvas_->CanUndo());
}

TEST_F(PolygonCanvasTest, MouseReleaseInDrawingModeCallsAddPoint)
{
  canvas_->SetAnnotationSet(&mock_);
  canvas_->SetDrawingMode(0);

  EXPECT_CALL(mock_, BeginSegment(0)).WillOnce(testing::Return(segcore::SegmentId{1}));
  EXPECT_CALL(mock_, AddPoint(segcore::SegmentId{1}, segcore::Point2D{10, 20}));

  QTest::mouseClick(canvas_, Qt::LeftButton, Qt::NoModifier, QPoint(10, 20));
}

TEST_F(PolygonCanvasTest, MouseReleaseRightButtonCallsCommitSegment)
{
  canvas_->SetAnnotationSet(&mock_);
  canvas_->SetDrawingMode(0);

  segcore::Segment ip;
  ip.id = 1;
  ip.points = {{0, 0}, {10, 0}, {5, 10}};

  ON_CALL(mock_, GetInProgress()).WillByDefault(testing::Return(&ip));
  EXPECT_CALL(mock_, CommitSegment(segcore::SegmentId{1})).WillOnce(testing::Return(true));

  QTest::mouseClick(canvas_, Qt::RightButton, Qt::NoModifier, QPoint(50, 50));
}

TEST_F(PolygonCanvasTest, DragMoveSetsStateThenCallsMovePoint)
{
  canvas_->SetAnnotationSet(&mock_);

  segcore::Segment ip;
  ip.id = 1;
  ip.points = {{10, 20}, {50, 20}, {30, 50}};

  ON_CALL(mock_, GetInProgress()).WillByDefault(testing::Return(&ip));
  EXPECT_CALL(mock_, MovePoint(segcore::SegmentId{1}, 0, segcore::Point2D{15, 25}));

  QTest::mousePress(canvas_, Qt::LeftButton, Qt::NoModifier, QPoint(10, 20));
  QTest::mouseRelease(canvas_, Qt::LeftButton, Qt::NoModifier, QPoint(15, 25));
}

TEST_F(PolygonCanvasTest, KeyPressCtrlZCallsUndo)
{
  canvas_->SetAnnotationSet(&mock_);
  canvas_->setFocus();
  ON_CALL(mock_, CanUndo()).WillByDefault(testing::Return(true));
  EXPECT_CALL(mock_, Undo());
  QTest::keyClick(canvas_, Qt::Key_Z, Qt::ControlModifier);
}

TEST_F(PolygonCanvasTest, UndoSkippedWhenCanUndoFalse)
{
  canvas_->SetAnnotationSet(&mock_);
  canvas_->setFocus();
  ON_CALL(mock_, CanUndo()).WillByDefault(testing::Return(false));
  EXPECT_CALL(mock_, Undo()).Times(0);
  QTest::keyClick(canvas_, Qt::Key_Z, Qt::ControlModifier);
}

TEST_F(PolygonCanvasTest, RedoSkippedWhenCanRedoFalse)
{
  canvas_->SetAnnotationSet(&mock_);
  canvas_->setFocus();
  ON_CALL(mock_, CanRedo()).WillByDefault(testing::Return(false));
  EXPECT_CALL(mock_, Redo()).Times(0);
  QTest::keyClick(canvas_, Qt::Key_Y, Qt::ControlModifier);
}

TEST_F(PolygonCanvasTest, PasteEmitsPolygonsChangedOnSuccess)
{
  canvas_->SetAnnotationSet(&mock_);
  canvas_->setFocus();
  ON_CALL(mock_, PasteSegment()).WillByDefault(testing::Return(segcore::SegmentId{7}));
  ON_CALL(mock_, GetSegments()).WillByDefault(testing::ReturnRef(empty_segments_));

  QSignalSpy spy(canvas_, &PolygonCanvas::PolygonsChanged);
  QTest::keyClick(canvas_, Qt::Key_V, Qt::ControlModifier);

  EXPECT_EQ(spy.count(), 1);
}

TEST_F(PolygonCanvasTest, PasteDoesNotEmitPolygonsChangedWhenClipboardEmpty)
{
  canvas_->SetAnnotationSet(&mock_);
  canvas_->setFocus();
  ON_CALL(mock_, PasteSegment()).WillByDefault(testing::Return(segcore::kInvalidSegmentId));

  QSignalSpy spy(canvas_, &PolygonCanvas::PolygonsChanged);
  QTest::keyClick(canvas_, Qt::Key_V, Qt::ControlModifier);

  EXPECT_EQ(spy.count(), 0);
}

TEST_F(PolygonCanvasTest, KeyPressDeleteCallsDeleteSegment)
{
  canvas_->SetAnnotationSet(&mock_);
  canvas_->setFocus();

  ON_CALL(mock_, GetSelectedSegmentId()).WillByDefault(testing::Return(segcore::SegmentId{42}));
  EXPECT_CALL(mock_, DeleteSegment(segcore::SegmentId{42}));

  QTest::keyClick(canvas_, Qt::Key_Delete);
}

TEST_F(PolygonCanvasTest, KeyPressCtrlCCallsCopySegment)
{
  canvas_->SetAnnotationSet(&mock_);
  canvas_->setFocus();

  ON_CALL(mock_, GetSelectedSegmentId()).WillByDefault(testing::Return(segcore::SegmentId{5}));
  EXPECT_CALL(mock_, CopySegment(segcore::SegmentId{5}));

  QTest::keyClick(canvas_, Qt::Key_C, Qt::ControlModifier);
}

TEST_F(PolygonCanvasTest, KeyPressCtrlVCallsPasteSegment)
{
  canvas_->SetAnnotationSet(&mock_);
  canvas_->setFocus();

  EXPECT_CALL(mock_, PasteSegment()).WillOnce(testing::Return(segcore::kInvalidSegmentId));

  QTest::keyClick(canvas_, Qt::Key_V, Qt::ControlModifier);
}

TEST_F(PolygonCanvasTest, CanUndoDelegatesToMock)
{
  canvas_->SetAnnotationSet(&mock_);
  EXPECT_CALL(mock_, CanUndo()).WillOnce(testing::Return(true));
  EXPECT_TRUE(canvas_->CanUndo());
}

TEST_F(PolygonCanvasTest, CanRedoDelegatesToMock)
{
  canvas_->SetAnnotationSet(&mock_);
  EXPECT_CALL(mock_, CanRedo()).WillOnce(testing::Return(false));
  EXPECT_FALSE(canvas_->CanRedo());
}

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}

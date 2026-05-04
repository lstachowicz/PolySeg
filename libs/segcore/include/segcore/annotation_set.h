#ifndef POLYSEG_SEGCORE_ANNOTATION_SET_H
#define POLYSEG_SEGCORE_ANNOTATION_SET_H

#include <optional>
#include <vector>
#include <segcore/history.h>
#include <segcore/iannotation_set.h>
#include <segcore/segment.h>
#include <segcore/types.h>

namespace segcore {

class AnnotationSet : public IAnnotationSet
{
 public:
  SegmentId BeginSegment(int class_id) override;
  void AddPoint(SegmentId id, Point2D p) override;
  void InsertPoint(SegmentId id, int index, Point2D p) override;
  void MovePoint(SegmentId id, int index, Point2D new_pos) override;
  void RemovePoint(SegmentId id, int index) override;
  bool CommitSegment(SegmentId id) override;
  void DeleteSegment(SegmentId id) override;
  void ClearAll() override;

  const Segment* GetSegment(SegmentId id) const override;
  const std::vector<Segment>& GetSegments() const override;
  const Segment* GetInProgress() const override;

  void SelectSegment(SegmentId id) override;
  void DeselectAll() override;
  SegmentId GetSelectedSegmentId() const override;

  void CopySegment(SegmentId id) override;
  SegmentId PasteSegment() override;
  bool HasClipboard() const override;
  const Segment* GetClipboard() const;
  void SetClipboard(const Segment* segment);

  void CancelInProgress() override;

  void Undo() override;
  void Redo() override;
  bool CanUndo() const override;
  bool CanRedo() const override;

 private:
  Segment* FindSegment(SegmentId id);

  std::vector<Segment> segments_;
  Segment in_progress_;
  bool has_in_progress_ = false;
  SegmentId next_id_ = 1;
  SegmentId selected_id_ = kInvalidSegmentId;
  std::optional<Segment> clipboard_;
  UndoStack undo_stack_;
};

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_ANNOTATION_SET_H

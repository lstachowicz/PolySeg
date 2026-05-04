#ifndef POLYSEG_SEGCORE_IANNOTATION_SET_H
#define POLYSEG_SEGCORE_IANNOTATION_SET_H

#include <segcore/segment.h>
#include <segcore/types.h>

#include <vector>

namespace segcore
{

class IAnnotationSet
{
 public:
  virtual ~IAnnotationSet() = default;

  virtual SegmentId BeginSegment(int class_id) = 0;
  virtual void AddPoint(SegmentId id, Point2D p) = 0;
  virtual void InsertPoint(SegmentId id, int index, Point2D p) = 0;
  virtual void MovePoint(SegmentId id, int index, Point2D new_pos) = 0;
  virtual void RemovePoint(SegmentId id, int index) = 0;
  virtual bool CommitSegment(SegmentId id) = 0;
  virtual void DeleteSegment(SegmentId id) = 0;
  virtual void ClearAll() = 0;

  virtual const Segment* GetSegment(SegmentId id) const = 0;
  virtual const std::vector<Segment>& GetSegments() const = 0;
  virtual const Segment* GetInProgress() const = 0;

  virtual void SelectSegment(SegmentId id) = 0;
  virtual void DeselectAll() = 0;
  virtual SegmentId GetSelectedSegmentId() const = 0;

  virtual void CopySegment(SegmentId id) = 0;
  virtual SegmentId PasteSegment() = 0;
  virtual bool HasClipboard() const = 0;

  virtual void CancelInProgress() = 0;

  virtual void Undo() = 0;
  virtual void Redo() = 0;
  virtual bool CanUndo() const = 0;
  virtual bool CanRedo() const = 0;
};

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_IANNOTATION_SET_H

#include <segcore/annotation_set.h>

#include <algorithm>

namespace segcore
{

Segment* AnnotationSet::FindSegment(SegmentId id)
{
  auto it = std::find_if(segments_.begin(), segments_.end(),
                         [id](const Segment& s) { return s.id == id; });
  return it != segments_.end() ? &(*it) : nullptr;
}

SegmentId AnnotationSet::BeginSegment(int class_id)
{
  SegmentId id = next_id_++;
  in_progress_ = Segment{};
  in_progress_.id = id;
  in_progress_.class_id = class_id;
  has_in_progress_ = true;
  return id;
}

void AnnotationSet::AddPoint(SegmentId id, Point2D p)
{
  if (has_in_progress_ && in_progress_.id == id)
  {
    undo_stack_.Push({[this, id]()
                      {
                        if (has_in_progress_ && in_progress_.id == id &&
                            !in_progress_.points.empty())
                        {
                          in_progress_.points.pop_back();
                        }
                      },
                      [this, id, p]()
                      {
                        if (has_in_progress_ && in_progress_.id == id)
                        {
                          in_progress_.points.push_back(p);
                        }
                      }});
    return;
  }
  Segment* seg = FindSegment(id);
  if (!seg)
  {
    return;
  }
  undo_stack_.Push({[this, id]()
                    {
                      Segment* s = FindSegment(id);
                      if (s && !s->points.empty())
                      {
                        s->points.pop_back();
                      }
                    },
                    [this, id, p]()
                    {
                      Segment* s = FindSegment(id);
                      if (s)
                      {
                        s->points.push_back(p);
                      }
                    }});
}

void AnnotationSet::InsertPoint(SegmentId id, int index, Point2D p)
{
  Segment* seg = FindSegment(id);
  if (!seg || index < 0 || index > static_cast<int>(seg->points.size()))
  {
    return;
  }
  undo_stack_.Push({[this, id, index]()
                    {
                      Segment* s = FindSegment(id);
                      if (s && index < static_cast<int>(s->points.size()))
                      {
                        s->points.erase(s->points.begin() + index);
                      }
                    },
                    [this, id, index, p]()
                    {
                      Segment* s = FindSegment(id);
                      if (s && index <= static_cast<int>(s->points.size()))
                      {
                        s->points.insert(s->points.begin() + index, p);
                      }
                    }});
}

void AnnotationSet::MovePoint(SegmentId id, int index, Point2D new_pos)
{
  if (has_in_progress_ && in_progress_.id == id)
  {
    if (index < 0 || index >= static_cast<int>(in_progress_.points.size()))
    {
      return;
    }
    undo_stack_.Push({[this, id, index, old_pos = in_progress_.points[index]]()
                      {
                        if (has_in_progress_ && in_progress_.id == id &&
                            index < static_cast<int>(in_progress_.points.size()))
                        {
                          in_progress_.points[index] = old_pos;
                        }
                      },
                      [this, id, index, new_pos]()
                      {
                        if (has_in_progress_ && in_progress_.id == id &&
                            index < static_cast<int>(in_progress_.points.size()))
                        {
                          in_progress_.points[index] = new_pos;
                        }
                      }});
    return;
  }
  Segment* seg = FindSegment(id);
  if (!seg || index < 0 || index >= static_cast<int>(seg->points.size()))
  {
    return;
  }
  Point2D old_pos = seg->points[index];
  undo_stack_.Push({[this, id, index, old_pos]()
                    {
                      Segment* s = FindSegment(id);
                      if (s && index < static_cast<int>(s->points.size()))
                      {
                        s->points[index] = old_pos;
                      }
                    },
                    [this, id, index, new_pos]()
                    {
                      Segment* s = FindSegment(id);
                      if (s && index < static_cast<int>(s->points.size()))
                      {
                        s->points[index] = new_pos;
                      }
                    }});
}

void AnnotationSet::CancelInProgress()
{
  if (has_in_progress_)
  {
    in_progress_ = Segment{};
    has_in_progress_ = false;
  }
}

void AnnotationSet::RemovePoint(SegmentId id, int index)
{
  Segment* seg = FindSegment(id);
  if (!seg || index < 0 || index >= static_cast<int>(seg->points.size()))
  {
    return;
  }
  Point2D removed = seg->points[index];
  undo_stack_.Push({[this, id, index, removed]()
                    {
                      Segment* s = FindSegment(id);
                      if (s && index <= static_cast<int>(s->points.size()))
                      {
                        s->points.insert(s->points.begin() + index, removed);
                      }
                    },
                    [this, id, index]()
                    {
                      Segment* s = FindSegment(id);
                      if (s && index < static_cast<int>(s->points.size()))
                      {
                        s->points.erase(s->points.begin() + index);
                      }
                    }});
}

bool AnnotationSet::CommitSegment(SegmentId id)
{
  if (!has_in_progress_ || in_progress_.id != id)
  {
    return false;
  }
  if (static_cast<int>(in_progress_.points.size()) < 3)
  {
    return false;
  }
  Segment to_commit = in_progress_;
  undo_stack_.Push(
      {[this, to_commit]()
       {
         segments_.erase(std::remove_if(segments_.begin(), segments_.end(),
                                        [&](const Segment& s) { return s.id == to_commit.id; }),
                         segments_.end());
         in_progress_ = to_commit;
         has_in_progress_ = true;
       },
       [this, to_commit]()
       {
         has_in_progress_ = false;
         in_progress_ = Segment{};
         auto it = std::find_if(segments_.begin(), segments_.end(),
                                [&](const Segment& s) { return s.id == to_commit.id; });
         if (it == segments_.end())
         {
           segments_.push_back(to_commit);
         }
       }});
  return true;
}

void AnnotationSet::DeleteSegment(SegmentId id)
{
  auto it = std::find_if(segments_.begin(), segments_.end(),
                         [id](const Segment& s) { return s.id == id; });
  if (it == segments_.end())
  {
    return;
  }
  Segment saved = *it;
  int saved_index = static_cast<int>(std::distance(segments_.begin(), it));
  if (selected_id_ == id)
  {
    selected_id_ = kInvalidSegmentId;
  }
  undo_stack_.Push({[this, saved, saved_index]()
                    {
                      int insert_at = std::min(saved_index, static_cast<int>(segments_.size()));
                      segments_.insert(segments_.begin() + insert_at, saved);
                    },
                    [this, id]()
                    {
                      segments_.erase(std::remove_if(segments_.begin(), segments_.end(),
                                                     [id](const Segment& s) { return s.id == id; }),
                                      segments_.end());
                    }});
}

void AnnotationSet::ClearAll()
{
  segments_.clear();
  in_progress_ = Segment{};
  has_in_progress_ = false;
  selected_id_ = kInvalidSegmentId;
  undo_stack_.Clear();
}

const Segment* AnnotationSet::GetSegment(SegmentId id) const
{
  auto it = std::find_if(segments_.begin(), segments_.end(),
                         [id](const Segment& s) { return s.id == id; });
  return it != segments_.end() ? &(*it) : nullptr;
}

const std::vector<Segment>& AnnotationSet::GetSegments() const
{
  return segments_;
}

const Segment* AnnotationSet::GetInProgress() const
{
  return has_in_progress_ ? &in_progress_ : nullptr;
}

void AnnotationSet::SelectSegment(SegmentId id)
{
  for (auto& seg : segments_)
  {
    seg.selected = (seg.id == id);
  }
  selected_id_ = id;
}

void AnnotationSet::DeselectAll()
{
  for (auto& seg : segments_)
  {
    seg.selected = false;
  }
  selected_id_ = kInvalidSegmentId;
}

SegmentId AnnotationSet::GetSelectedSegmentId() const
{
  return selected_id_;
}

void AnnotationSet::CopySegment(SegmentId id)
{
  const Segment* seg = GetSegment(id);
  if (seg)
  {
    clipboard_ = *seg;
  }
}

SegmentId AnnotationSet::PasteSegment()
{
  if (!clipboard_.has_value())
  {
    return kInvalidSegmentId;
  }
  Segment pasted = clipboard_.value();
  pasted.id = next_id_++;
  pasted.selected = false;
  undo_stack_.Push({[this, id = pasted.id]()
                    {
                      segments_.erase(std::remove_if(segments_.begin(), segments_.end(),
                                                     [id](const Segment& s) { return s.id == id; }),
                                      segments_.end());
                    },
                    [this, pasted]()
                    {
                      auto it = std::find_if(segments_.begin(), segments_.end(),
                                             [&](const Segment& s) { return s.id == pasted.id; });
                      if (it == segments_.end())
                        segments_.push_back(pasted);
                    }});
  return pasted.id;
}

bool AnnotationSet::HasClipboard() const
{
  return clipboard_.has_value();
}

const Segment* AnnotationSet::GetClipboard() const
{
  if (clipboard_.has_value())
  {
    return &clipboard_.value();
  }
  return nullptr;
}

void AnnotationSet::SetClipboard(const Segment* segment)
{
  if (segment != nullptr)
  {
    clipboard_ = *segment;
  }
  else
  {
    clipboard_.reset();
  }
}

void AnnotationSet::Undo()
{
  undo_stack_.Undo();
}

void AnnotationSet::Redo()
{
  undo_stack_.Redo();
}

bool AnnotationSet::CanUndo() const
{
  return undo_stack_.CanUndo();
}

bool AnnotationSet::CanRedo() const
{
  return undo_stack_.CanRedo();
}

}  // namespace segcore

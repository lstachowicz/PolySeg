#include <segcore/history.h>

namespace segcore
{

void UndoStack::Push(Command cmd)
{
  cmd.redo();
  redo_.clear();
  undo_.push_back(std::move(cmd));
  if (static_cast<int>(undo_.size()) > kMaxHistory)
  {
    undo_.pop_front();
  }
}

void UndoStack::Undo()
{
  if (!CanUndo())
  {
    return;
  }
  Command cmd = std::move(undo_.back());
  undo_.pop_back();
  cmd.undo();
  redo_.push_back(std::move(cmd));
}

void UndoStack::Redo()
{
  if (!CanRedo())
  {
    return;
  }
  Command cmd = std::move(redo_.back());
  redo_.pop_back();
  cmd.redo();
  undo_.push_back(std::move(cmd));
}

bool UndoStack::CanUndo() const
{
  return !undo_.empty();
}

bool UndoStack::CanRedo() const
{
  return !redo_.empty();
}

void UndoStack::Clear()
{
  undo_.clear();
  redo_.clear();
}

}  // namespace segcore

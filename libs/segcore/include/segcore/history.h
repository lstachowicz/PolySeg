#ifndef POLYSEG_SEGCORE_HISTORY_H
#define POLYSEG_SEGCORE_HISTORY_H

#include <deque>
#include <functional>

namespace segcore
{

struct Command
{
  std::function<void()> undo;
  std::function<void()> redo;
};

class UndoStack
{
 public:
  void Push(Command cmd);
  void Undo();
  void Redo();
  bool CanUndo() const;
  bool CanRedo() const;
  void Clear();

 private:
  static constexpr int kMaxHistory = 50;
  std::deque<Command> undo_;
  std::deque<Command> redo_;
};

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_HISTORY_H

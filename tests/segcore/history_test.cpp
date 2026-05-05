#include <gtest/gtest.h>
#include <segcore/history.h>

using segcore::Command;
using segcore::UndoStack;

TEST(UndoStackTest, EmptyStackCannotUndoOrRedo)
{
  UndoStack stack;
  EXPECT_FALSE(stack.CanUndo());
  EXPECT_FALSE(stack.CanRedo());
}

TEST(UndoStackTest, PushEnablesUndo)
{
  UndoStack stack;
  bool redo_called = false;
  stack.Push({[] {}, [&] { redo_called = true; }});
  EXPECT_TRUE(redo_called);
  EXPECT_TRUE(stack.CanUndo());
  EXPECT_FALSE(stack.CanRedo());
}

TEST(UndoStackTest, UndoFiresUndoCallback)
{
  UndoStack stack;
  bool undo_called = false;
  stack.Push({[&] { undo_called = true; }, [] {}});
  stack.Undo();
  EXPECT_TRUE(undo_called);
  EXPECT_FALSE(stack.CanUndo());
  EXPECT_TRUE(stack.CanRedo());
}

TEST(UndoStackTest, RedoFiresRedoCallback)
{
  UndoStack stack;
  bool redo_called = false;
  stack.Push({[] {}, [&] { redo_called = true; }});
  redo_called = false;
  stack.Undo();
  stack.Redo();
  EXPECT_TRUE(redo_called);
  EXPECT_TRUE(stack.CanUndo());
  EXPECT_FALSE(stack.CanRedo());
}

TEST(UndoStackTest, PushDropsRedoHistory)
{
  UndoStack stack;
  stack.Push({[] {}, [] {}});
  stack.Undo();
  EXPECT_TRUE(stack.CanRedo());
  stack.Push({[] {}, [] {}});
  EXPECT_FALSE(stack.CanRedo());
}

TEST(UndoStackTest, Push51DropsOldest)
{
  UndoStack stack;
  int undo_count = 0;
  for (int i = 0; i < 51; ++i)
  {
    stack.Push({[&] { ++undo_count; }, [] {}});
  }
  while (stack.CanUndo())
  {
    stack.Undo();
  }
  EXPECT_EQ(undo_count, 50);
}

TEST(UndoStackTest, ClearResetsAll)
{
  UndoStack stack;
  stack.Push({[] {}, [] {}});
  stack.Undo();
  stack.Clear();
  EXPECT_FALSE(stack.CanUndo());
  EXPECT_FALSE(stack.CanRedo());
}

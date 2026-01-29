#ifndef SHORTCUTEDITDIALOG_H
#define SHORTCUTEDITDIALOG_H

#include <QDialog>

namespace Ui
{
class ShortcutEditDialog;
}

/**
 * @brief Dialog for editing a single keyboard shortcut
 *
 * Provides a simple interface for capturing a new key sequence
 * for a specific action.
 */
class ShortcutEditDialog : public QDialog
{
  Q_OBJECT

 public:
  explicit ShortcutEditDialog(const QString& action, const QString& current_shortcut,
                              QWidget* parent = nullptr);
  ~ShortcutEditDialog();

  QString GetKeySequence() const;

 private:
  Ui::ShortcutEditDialog* ui_;
};

#endif  // SHORTCUTEDITDIALOG_H

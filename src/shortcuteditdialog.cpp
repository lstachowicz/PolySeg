#include "shortcuteditdialog.h"

#include "ui_shortcuteditdialog.h"

ShortcutEditDialog::ShortcutEditDialog(const QString& action, const QString& current_shortcut,
                                       QWidget* parent)
    : QDialog(parent), ui_(new Ui::ShortcutEditDialog)
{
  ui_->setupUi(this);
  setWindowTitle("Edit Shortcut: " + action);

  ui_->key_edit_->setKeySequence(QKeySequence(current_shortcut));

  connect(ui_->clear_button_, &QPushButton::clicked, ui_->key_edit_, &QKeySequenceEdit::clear);
}

ShortcutEditDialog::~ShortcutEditDialog()
{
  delete ui_;
}

QString ShortcutEditDialog::GetKeySequence() const
{
  return ui_->key_edit_->keySequence().toString();
}

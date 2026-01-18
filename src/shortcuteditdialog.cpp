#include "shortcuteditdialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ShortcutEditDialog::ShortcutEditDialog(const QString& action, const QString& current_shortcut,
                                       QWidget* parent)
    : QDialog(parent)
{
  setWindowTitle("Edit Shortcut: " + action);
  setMinimumWidth(300);

  QVBoxLayout* layout = new QVBoxLayout(this);

  QLabel* label = new QLabel("Press new key combination:");
  layout->addWidget(label);

  key_edit_ = new QKeySequenceEdit();
  key_edit_->setKeySequence(QKeySequence(current_shortcut));
  layout->addWidget(key_edit_);

  QHBoxLayout* btn_layout = new QHBoxLayout();
  btn_layout->addStretch();

  QPushButton* clear_btn = new QPushButton("Clear");
  connect(clear_btn, &QPushButton::clicked, key_edit_, &QKeySequenceEdit::clear);
  btn_layout->addWidget(clear_btn);

  QPushButton* ok_btn = new QPushButton("OK");
  connect(ok_btn, &QPushButton::clicked, this, &QDialog::accept);
  btn_layout->addWidget(ok_btn);

  QPushButton* cancel_btn = new QPushButton("Cancel");
  connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);
  btn_layout->addWidget(cancel_btn);

  layout->addLayout(btn_layout);
}

QString ShortcutEditDialog::GetKeySequence() const
{
  return key_edit_->keySequence().toString();
}

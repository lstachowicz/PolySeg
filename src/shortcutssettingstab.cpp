#include "shortcutssettingstab.h"

#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

#include "shortcuteditdialog.h"

ShortcutsSettingsTab::ShortcutsSettingsTab(QWidget* parent) : BaseSettingsTab(parent)
{
  InitializeDefaultShortcuts();
  shortcuts_ = default_shortcuts_;
}

void ShortcutsSettingsTab::InitializeDefaultShortcuts()
{
  default_shortcuts_["New Project"] = "Ctrl+N";
  default_shortcuts_["Open Project"] = "Ctrl+O";
  default_shortcuts_["Save"] = "Ctrl+S";
  default_shortcuts_["Undo"] = "Ctrl+Z";
  default_shortcuts_["Redo"] = "Ctrl+Y";
  default_shortcuts_["Copy Polygon"] = "Ctrl+C";
  default_shortcuts_["Paste Polygon"] = "Ctrl+V";
  default_shortcuts_["Delete Selected"] = "Del";
  default_shortcuts_["Zoom In"] = "Ctrl+=";
  default_shortcuts_["Zoom Out"] = "Ctrl+-";
  default_shortcuts_["Reset Zoom"] = "Ctrl+0";
  default_shortcuts_["Next Class"] = "Tab";
  default_shortcuts_["Previous Class"] = "Shift+Tab";
  default_shortcuts_["Next Image"] = "Right";
  default_shortcuts_["Previous Image"] = "Left";
  default_shortcuts_["First Image"] = "Home";
  default_shortcuts_["Last Image"] = "End";
  default_shortcuts_["Auto Detect"] = "Ctrl+D";
  default_shortcuts_["Batch Detect"] = "Ctrl+Shift+D";
  default_shortcuts_["Next Unreviewed"] = "Ctrl+U";
  default_shortcuts_["Approve & Save"] = "Ctrl+Return";
  default_shortcuts_["Reject & Clear"] = "Ctrl+Backspace";
  default_shortcuts_["Keyboard Shortcuts"] = "F1";
}

void ShortcutsSettingsTab::SetupUI()
{
  QVBoxLayout* main_layout = GetMainLayout();

  // Info section
  QLabel* info = new QLabel(
      "Click on a shortcut cell to edit it. Press a key combination to assign a new shortcut.");
  info->setWordWrap(true);
  info->setStyleSheet("color: gray; font-size: 10px;");
  main_layout->addWidget(info);

  // Shortcuts table
  QGroupBox* shortcuts_group = new QGroupBox("Keyboard Shortcuts");
  QVBoxLayout* group_layout = new QVBoxLayout(shortcuts_group);

  table_ = new QTableWidget();
  table_->setColumnCount(3);
  table_->setHorizontalHeaderLabels({"Action", "Shortcut", "Default"});
  table_->horizontalHeader()->setStretchLastSection(true);
  table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

  group_layout->addWidget(table_);

  // Reset button
  QHBoxLayout* button_layout = new QHBoxLayout();
  button_layout->addStretch();

  reset_button_ = new QPushButton("Reset to Defaults");
  button_layout->addWidget(reset_button_);

  group_layout->addLayout(button_layout);

  main_layout->addWidget(shortcuts_group);
  main_layout->addStretch();
}

void ShortcutsSettingsTab::ConnectSignals()
{
  connect(table_, &QTableWidget::cellClicked, this, &ShortcutsSettingsTab::OnCellClicked);
  connect(reset_button_, &QPushButton::clicked, this, &ShortcutsSettingsTab::OnResetDefaults);
}

void ShortcutsSettingsTab::LoadShortcuts()
{
  QSettings settings("PolySeg", "PolySeg");
  settings.beginGroup("Shortcuts");

  QStringList keys = settings.childKeys();
  if (keys.isEmpty())
  {
    // Use defaults
    shortcuts_ = default_shortcuts_;
  }
  else
  {
    // Merge with defaults - keep custom values, add new defaults
    for (auto it = default_shortcuts_.begin(); it != default_shortcuts_.end(); ++it)
    {
      if (settings.contains(it.key()))
      {
        shortcuts_[it.key()] = settings.value(it.key()).toString();
      }
      else
      {
        shortcuts_[it.key()] = it.value();
      }
    }
  }

  settings.endGroup();
  PopulateTable();
}

void ShortcutsSettingsTab::SaveShortcuts()
{
  QSettings settings("PolySeg", "PolySeg");
  settings.beginGroup("Shortcuts");

  for (auto it = shortcuts_.begin(); it != shortcuts_.end(); ++it)
  {
    settings.setValue(it.key(), it.value());
  }

  settings.endGroup();
  emit shortcutsChanged(shortcuts_);
}

void ShortcutsSettingsTab::PopulateTable()
{
  table_->setRowCount(shortcuts_.size());

  int row = 0;
  for (auto it = shortcuts_.begin(); it != shortcuts_.end(); ++it)
  {
    // Action name
    QTableWidgetItem* action_item = new QTableWidgetItem(it.key());
    table_->setItem(row, 0, action_item);

    // Current shortcut
    QTableWidgetItem* shortcut_item = new QTableWidgetItem(it.value());
    shortcut_item->setTextAlignment(Qt::AlignCenter);
    table_->setItem(row, 1, shortcut_item);

    // Default shortcut
    QString default_sc = default_shortcuts_.value(it.key(), "");
    QTableWidgetItem* default_item = new QTableWidgetItem(default_sc);
    default_item->setTextAlignment(Qt::AlignCenter);
    default_item->setFlags(default_item->flags() & ~Qt::ItemIsEditable);
    default_item->setForeground(QBrush(Qt::gray));
    table_->setItem(row, 2, default_item);

    row++;
  }
}

void ShortcutsSettingsTab::OnCellClicked(int row, int column)
{
  if (column != 1)
    return;  // Only edit shortcut column

  QString action = table_->item(row, 0)->text();
  QString current = shortcuts_.value(action);

  ShortcutEditDialog dialog(action, current, this);

  if (dialog.exec() == QDialog::Accepted)
  {
    QString new_shortcut = dialog.GetKeySequence();

    if (ValidateShortcut(new_shortcut, row))
    {
      shortcuts_[action] = new_shortcut;
      table_->item(row, 1)->setText(new_shortcut);
    }
  }
}

void ShortcutsSettingsTab::OnResetDefaults()
{
  QMessageBox::StandardButton reply =
      QMessageBox::question(this, "Reset Shortcuts", "Reset all shortcuts to default values?",
                            QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes)
  {
    shortcuts_ = default_shortcuts_;
    PopulateTable();
  }
}

bool ShortcutsSettingsTab::ValidateShortcut(const QString& shortcut, int current_row)
{
  if (shortcut.isEmpty())
    return true;  // Allow clearing shortcut

  // Check for conflicts
  int row = 0;
  for (auto it = shortcuts_.begin(); it != shortcuts_.end(); ++it)
  {
    if (row != current_row && it.value() == shortcut)
    {
      QMessageBox::warning(this, "Conflict",
                           QString("Shortcut '%1' is already assigned to '%2'")
                               .arg(shortcut)
                               .arg(it.key()));
      return false;
    }
    row++;
  }

  return true;
}

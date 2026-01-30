#ifndef SHORTCUTSSETTINGSTAB_H
#define SHORTCUTSSETTINGSTAB_H

#include "settingstabbase.h"

#include <QMap>

namespace Ui
{
class ShortcutsSettingsTab;
}

/**
 * @brief Settings tab for keyboard shortcuts configuration
 *
 * This tab allows users to customize keyboard shortcuts for all actions.
 * Unlike other tabs, shortcuts are stored globally in QSettings (not per-project),
 * so LoadFromConfig/SaveToConfig are no-ops.
 */
class ShortcutsSettingsTab : public BaseSettingsTab
{
  Q_OBJECT

 public:
  explicit ShortcutsSettingsTab(QWidget* parent = nullptr);
  ~ShortcutsSettingsTab() override;

  // BaseSettingsTab interface (no-op - shortcuts use QSettings, not ProjectConfig)
  void LoadFromConfig(const ProjectConfig&) override {}
  void SaveToConfig(ProjectConfig&) override {}

  // Shortcuts-specific methods
  void LoadShortcuts();
  void SaveShortcuts();
  QMap<QString, QString> GetShortcuts() const { return shortcuts_; }

 signals:
  void shortcutsChanged(const QMap<QString, QString>& shortcuts);

 private slots:
  void OnCellClicked(int row, int column);
  void OnResetDefaults();

 private:
  void InitializeDefaultShortcuts();
  void PopulateTable();
  bool ValidateShortcut(const QString& shortcut, int current_row);

  Ui::ShortcutsSettingsTab* ui_;
  QMap<QString, QString> shortcuts_;
  QMap<QString, QString> default_shortcuts_;
};

#endif  // SHORTCUTSSETTINGSTAB_H

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QList>
#include <QMap>

#include "projectconfig.h"
#include "settingstabbase.h"

namespace Ui
{
class SettingsDialog;
}

class ShortcutsSettingsTab;

class SettingsDialog : public QDialog
{
  Q_OBJECT

 public:
  explicit SettingsDialog(ProjectConfig& config, const QString& project_dir,
                         QWidget* parent = nullptr);
  ~SettingsDialog();

  ProjectConfig GetConfig() const { return config_; }

  void SetCurrentTab(int index);
  int GetShortcutsTabIndex() const;

 signals:
  void RequestModelRegistration();
  void RequestPluginWizard();
  void ShortcutsChanged(const QMap<QString, QString>& shortcuts);

 public slots:
  void RefreshModelList();

 private slots:
  void OnApply();
  void OnSave();

 private:
  void SetupTabs();
  void LoadAllTabs();
  void SaveAllTabs();
  void ConnectSignals();

  Ui::SettingsDialog* ui_;
  ProjectConfig config_;
  ProjectConfig original_config_;
  QString project_dir_;

  QList<BaseSettingsTab*> tabs_;
  ShortcutsSettingsTab* shortcuts_tab_;
};

#endif  // SETTINGSDIALOG_H

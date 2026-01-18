#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QList>
#include <QMap>
#include <QPushButton>
#include <QTabWidget>

#include "projectconfig.h"
#include "settingstabbase.h"

class ShortcutsSettingsTab;

class SettingsDialog : public QDialog
{
  Q_OBJECT

 public:
  explicit SettingsDialog(ProjectConfig& config, const QString& project_dir,
                         QWidget* parent = nullptr);

  ProjectConfig GetConfig() const { return config_; }

  void SetCurrentTab(int index);
  int GetShortcutsTabIndex() const;

 signals:
  void RequestModelRegistration();
  void ShortcutsChanged(const QMap<QString, QString>& shortcuts);

 public slots:
  void RefreshModelList();

 private slots:
  void OnApply();
  void OnSave();

 private:
  void SetupUI();
  void LoadAllTabs();
  void SaveAllTabs();

  ProjectConfig config_;
  ProjectConfig original_config_;
  QString project_dir_;

  QTabWidget* tab_widget_;
  QList<BaseSettingsTab*> tabs_;
  ShortcutsSettingsTab* shortcuts_tab_;

  QPushButton* apply_button_;
  QPushButton* save_button_;
  QPushButton* cancel_button_;
};

#endif  // SETTINGSDIALOG_H

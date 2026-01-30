#ifndef AIMODELSETTINGSTAB_H
#define AIMODELSETTINGSTAB_H

#include "settingstabbase.h"

#include <QMap>
#include <QString>

namespace Ui
{
class AIModelSettingsTab;
}

class ProjectConfig;

/**
 * @brief AI/Model settings tab for plugin configuration and model version tracking
 *
 * Responsibilities:
 * - AI Plugin Configuration (all fields)
 * - Plugin Settings (dynamic key-value pairs)
 * - Dataset Splits (train/val/test ratios and management)
 * - Model Versions tracking (table + compare)
 */
class AIModelSettingsTab : public BaseSettingsTab
{
  Q_OBJECT

 public:
  explicit AIModelSettingsTab(ProjectConfig& config, const QString& project_dir,
                              QWidget* parent = nullptr);
  ~AIModelSettingsTab() override;

  // BaseSettingsTab interface
  void LoadFromConfig(const ProjectConfig& config) override;
  void SaveToConfig(ProjectConfig& config) override;

 signals:
  /**
   * @brief Signal forwarded to parent dialog to trigger model registration
   */
  void requestModelRegistration();

  /**
   * @brief Signal emitted when split configuration changes
   */
  void splitsChanged();

  /**
   * @brief Signal emitted to request opening the plugin wizard
   */
  void requestPluginWizard();

 public slots:
  /**
   * @brief Refresh model versions table from config
   */
  void RefreshModelList();

 private slots:
  void OnBrowsePluginScript();
  void OnAddPluginSetting();
  void OnRemovePluginSetting();
  void OnSplitsEnabledChanged(int state);
  void OnSplitRatioChanged();
  void OnResetSplits();
  void UpdateSplitStatistics();
  void OnAddModelVersion();
  void OnEditModelNotes();
  void OnCompareModels();
  void OnRemoveModelVersion();

 private:
  void PopulatePluginSettingsTable();
  QMap<QString, QString> GetPluginSettingsFromTable() const;

  Ui::AIModelSettingsTab* ui_;
  ProjectConfig& config_;
  QString project_dir_;
};

#endif  // AIMODELSETTINGSTAB_H

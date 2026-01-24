#ifndef AIMODELSETTINGSTAB_H
#define AIMODELSETTINGSTAB_H

#include "settingstabbase.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QTableWidget>

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
  ~AIModelSettingsTab() override = default;

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

 public slots:
  /**
   * @brief Refresh model versions table from config
   */
  void RefreshModelList();

 protected:
  // BaseSettingsTab interface
  void SetupUI() override;
  void ConnectSignals() override;

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
  ProjectConfig& config_;
  QString project_dir_;

  // Plugin Configuration widgets
  QCheckBox* plugin_enabled_checkbox_;
  QLineEdit* plugin_name_edit_;
  QLineEdit* plugin_env_setup_edit_;
  QLineEdit* plugin_command_edit_;
  QLineEdit* plugin_script_edit_;
  QPushButton* browse_script_button_;
  QLineEdit* plugin_detect_args_edit_;
  QLineEdit* plugin_train_args_edit_;
  QFormLayout* plugin_settings_layout_;
  QMap<QString, QLineEdit*> plugin_setting_edits_;
  QPushButton* add_plugin_setting_button_;

  // Dataset Split widgets
  QCheckBox* splits_enabled_checkbox_;
  QSlider* train_ratio_slider_;
  QSlider* val_ratio_slider_;
  QSlider* test_ratio_slider_;
  QLabel* train_ratio_label_;
  QLabel* val_ratio_label_;
  QLabel* test_ratio_label_;
  QLabel* split_statistics_label_;
  QLineEdit* salt_edit_;
  QPushButton* reset_splits_button_;

  // Model Versions widgets
  QTableWidget* model_versions_table_;
  QPushButton* add_model_button_;
  QPushButton* edit_notes_button_;
  QPushButton* compare_models_button_;
  QPushButton* remove_model_button_;
};

#endif  // AIMODELSETTINGSTAB_H

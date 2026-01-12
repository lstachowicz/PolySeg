#ifndef PROJECTSETTINGSDIALOG_H
#define PROJECTSETTINGSDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>

#include "projectconfig.h"

class ProjectSettingsDialog : public QDialog
{
  Q_OBJECT

 public:
  explicit ProjectSettingsDialog(ProjectConfig& config, QWidget* parent = nullptr);
  ~ProjectSettingsDialog();

  ProjectConfig GetConfig() const { return config_; }

 private slots:
  void OnApply();
  void OnSave();

 private:
  void SetupUI();
  void SetupGeneralTab();
  void SetupAIModelTab();
  void SetupExportTab();
  void SetupAdvancedTab();
  void LoadFromConfig();
  void SaveToConfig();

  ProjectConfig config_;
  ProjectConfig original_config_;

  // Tab widgets
  QTabWidget* tab_widget_;

  // General tab
  QWidget* general_tab_;
  QLineEdit* project_name_edit_;
  QCheckBox* auto_save_checkbox_;
  QSpinBox* auto_save_interval_spinbox_;

  // AI Model tab
  QWidget* ai_model_tab_;
  QLineEdit* model_path_edit_;
  QPushButton* browse_model_button_;
  QSlider* confidence_slider_;
  QLabel* confidence_value_label_;
  QLabel* model_info_label_;
  QPushButton* download_model_button_;

  // Export tab
  QWidget* export_tab_;
  QComboBox* export_format_combo_;
  QCheckBox* normalize_coords_checkbox_;
  QSpinBox* coord_precision_spinbox_;

  // Advanced tab
  QWidget* advanced_tab_;
  QLineEdit* image_extensions_edit_;
  QLineEdit* images_folder_edit_;
  QLineEdit* labels_folder_edit_;
  QLineEdit* models_folder_edit_;
  QPushButton* browse_images_button_;
  QPushButton* browse_labels_button_;
  QPushButton* browse_models_button_;

  // Dialog buttons
  QPushButton* apply_button_;
  QPushButton* save_button_;
  QPushButton* cancel_button_;
};

#endif  // PROJECTSETTINGSDIALOG_H

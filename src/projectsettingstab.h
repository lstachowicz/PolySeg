#ifndef PROJECTSETTINGSTAB_H
#define PROJECTSETTINGSTAB_H

#include "settingstabbase.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTableWidget>

class ProjectConfig;

/**
 * @brief Settings tab for project configuration
 *
 * Manages:
 * - Project name and annotation type
 * - Auto-save settings
 * - Annotation classes (add/edit/remove/reorder)
 * - Custom folder paths (images, labels)
 */
class ProjectSettingsTab : public BaseSettingsTab
{
  Q_OBJECT

 public:
  explicit ProjectSettingsTab(ProjectConfig& config, const QString& project_dir,
                             QWidget* parent = nullptr);
  ~ProjectSettingsTab() override = default;

  // BaseSettingsTab interface
  void LoadFromConfig(const ProjectConfig& config) override;
  void SaveToConfig(ProjectConfig& config) override;

 signals:
  void classesChanged();

 protected:
  void SetupUI() override;
  void ConnectSignals() override;

 private slots:
  void OnAddClass();
  void OnEditClass();
  void OnRemoveClass();
  void OnMoveClassUp();
  void OnMoveClassDown();
  void OnBrowseImagesFolder();
  void OnBrowseLabelsFolder();

 private:
  ProjectConfig& config_;
  QString project_dir_;

  // Basic settings
  QLineEdit* project_name_edit_;
  QComboBox* annotation_type_combo_;

  // Auto-save settings
  QCheckBox* auto_save_checkbox_;
  QSpinBox* auto_save_interval_spinbox_;

  // Classes
  QTableWidget* classes_table_;
  QPushButton* add_class_button_;
  QPushButton* edit_class_button_;
  QPushButton* remove_class_button_;
  QPushButton* move_up_button_;
  QPushButton* move_down_button_;

  // Custom folder paths
  QLineEdit* images_folder_edit_;
  QLineEdit* labels_folder_edit_;
  QPushButton* browse_images_button_;
  QPushButton* browse_labels_button_;
};

#endif  // PROJECTSETTINGSTAB_H

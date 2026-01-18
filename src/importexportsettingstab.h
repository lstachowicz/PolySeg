#ifndef IMPORTEXPORTSETTINGSTAB_H
#define IMPORTEXPORTSETTINGSTAB_H

#include "settingstabbase.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>

class ProjectConfig;

/**
 * @brief Settings tab for Import/Export configuration
 *
 * Handles:
 * - Image crop preprocessing settings
 * - Import path configuration (base path, skip folders)
 * - Image extensions
 * - Export format selection
 * - Coordinate normalization and precision
 */
class ImportExportSettingsTab : public BaseSettingsTab
{
  Q_OBJECT

 public:
  explicit ImportExportSettingsTab(ProjectConfig& config, QWidget* parent = nullptr);
  ~ImportExportSettingsTab() override = default;

  void LoadFromConfig(const ProjectConfig& config) override;
  void SaveToConfig(ProjectConfig& config) override;

 protected:
  void SetupUI() override;
  void ConnectSignals() override;

 private slots:
  void OnBrowseImportBasePath();
  void OnAddSkipFolder();
  void OnRemoveSkipFolder();

 private:
  ProjectConfig& config_;

  // Import settings - Image Crop
  QCheckBox* crop_enabled_checkbox_;
  QSpinBox* crop_x_spinbox_;
  QSpinBox* crop_y_spinbox_;
  QSpinBox* crop_width_spinbox_;
  QSpinBox* crop_height_spinbox_;

  // Import settings - Path Configuration
  QLineEdit* import_base_path_edit_;
  QPushButton* browse_import_base_path_button_;
  QListWidget* skip_folders_list_;
  QPushButton* add_skip_folder_button_;
  QPushButton* remove_skip_folder_button_;

  // Import settings - Image Extensions
  QLineEdit* image_extensions_edit_;

  // Export settings
  QComboBox* export_format_combo_;
  QCheckBox* normalize_coords_checkbox_;
  QSpinBox* coord_precision_spinbox_;
};

#endif  // IMPORTEXPORTSETTINGSTAB_H

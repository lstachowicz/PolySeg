#include "importexportsettingstab.h"

#include <QFileDialog>
#include <QInputDialog>

#include "projectconfig.h"
#include "ui_importexportsettingstab.h"

ImportExportSettingsTab::ImportExportSettingsTab(ProjectConfig& config, QWidget* parent)
    : BaseSettingsTab(parent), ui_(new Ui::ImportExportSettingsTab), config_(config)
{
  ui_->setupUi(this);

  connect(ui_->browse_import_base_path_button_, &QPushButton::clicked, this,
          &ImportExportSettingsTab::OnBrowseImportBasePath);

  connect(ui_->add_skip_folder_button_, &QPushButton::clicked, this,
          &ImportExportSettingsTab::OnAddSkipFolder);

  connect(ui_->remove_skip_folder_button_, &QPushButton::clicked, this,
          &ImportExportSettingsTab::OnRemoveSkipFolder);
}

ImportExportSettingsTab::~ImportExportSettingsTab()
{
  delete ui_;
}

void ImportExportSettingsTab::LoadFromConfig(const ProjectConfig& config)
{
  // Load export settings (defaults for now)
  ui_->export_format_combo_->setCurrentIndex(0);
  ui_->normalize_coords_checkbox_->setChecked(true);
  ui_->coord_precision_spinbox_->setValue(6);

  // Load crop configuration
  const CropConfig& crop_cfg = config.GetCropConfig();
  ui_->crop_enabled_checkbox_->setChecked(crop_cfg.enabled);
  ui_->crop_x_spinbox_->setValue(crop_cfg.x);
  ui_->crop_y_spinbox_->setValue(crop_cfg.y);
  ui_->crop_width_spinbox_->setValue(crop_cfg.width);
  ui_->crop_height_spinbox_->setValue(crop_cfg.height);

  // Load import path configuration
  const ImportPathConfig& import_path_cfg = config.GetImportPathConfig();
  ui_->import_base_path_edit_->setText(import_path_cfg.base_path);
  ui_->skip_folders_list_->clear();
  for (const QString& folder : import_path_cfg.skip_folders)
  {
    ui_->skip_folders_list_->addItem(folder);
  }

  // Load image extensions (default for now)
  ui_->image_extensions_edit_->setText("jpg, jpeg, png, bmp, tiff");
}

void ImportExportSettingsTab::SaveToConfig(ProjectConfig& config)
{
  // Save crop configuration
  CropConfig crop_cfg;
  crop_cfg.enabled = ui_->crop_enabled_checkbox_->isChecked();
  crop_cfg.x = ui_->crop_x_spinbox_->value();
  crop_cfg.y = ui_->crop_y_spinbox_->value();
  crop_cfg.width = ui_->crop_width_spinbox_->value();
  crop_cfg.height = ui_->crop_height_spinbox_->value();
  config.SetCropConfig(crop_cfg);

  // Save import path configuration
  ImportPathConfig import_path_cfg;
  import_path_cfg.base_path = ui_->import_base_path_edit_->text();
  import_path_cfg.skip_folders.clear();
  for (int i = 0; i < ui_->skip_folders_list_->count(); ++i)
  {
    import_path_cfg.skip_folders.append(ui_->skip_folders_list_->item(i)->text());
  }
  config.SetImportPathConfig(import_path_cfg);
}

void ImportExportSettingsTab::OnBrowseImportBasePath()
{
  QString dir = QFileDialog::getExistingDirectory(this, "Select Base Path");
  if (!dir.isEmpty())
  {
    ui_->import_base_path_edit_->setText(dir);
  }
}

void ImportExportSettingsTab::OnAddSkipFolder()
{
  bool ok;
  QString folder =
      QInputDialog::getText(this, "Add Folder to Skip", "Folder name:", QLineEdit::Normal, "", &ok);
  if (ok && !folder.isEmpty())
  {
    ui_->skip_folders_list_->addItem(folder);
  }
}

void ImportExportSettingsTab::OnRemoveSkipFolder()
{
  QListWidgetItem* item = ui_->skip_folders_list_->currentItem();
  if (item)
  {
    delete item;
  }
}

#include "importexportsettingstab.h"
#include "projectconfig.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

ImportExportSettingsTab::ImportExportSettingsTab(ProjectConfig& config, QWidget* parent)
    : BaseSettingsTab(parent),
      config_(config),
      crop_enabled_checkbox_(nullptr),
      crop_x_spinbox_(nullptr),
      crop_y_spinbox_(nullptr),
      crop_width_spinbox_(nullptr),
      crop_height_spinbox_(nullptr),
      import_base_path_edit_(nullptr),
      browse_import_base_path_button_(nullptr),
      skip_folders_list_(nullptr),
      add_skip_folder_button_(nullptr),
      remove_skip_folder_button_(nullptr),
      image_extensions_edit_(nullptr),
      export_format_combo_(nullptr),
      normalize_coords_checkbox_(nullptr),
      coord_precision_spinbox_(nullptr)
{
}

void ImportExportSettingsTab::SetupUI()
{
  QVBoxLayout* main_layout = GetMainLayout();

  // ===== Import Settings =====
  QGroupBox* import_group = new QGroupBox("Import Settings");
  QVBoxLayout* import_layout = new QVBoxLayout(import_group);

  // Image Crop
  QLabel* crop_label = new QLabel("<b>Image Crop (Preprocessing)</b>");
  import_layout->addWidget(crop_label);

  crop_enabled_checkbox_ = new QCheckBox("Enable automatic image cropping on load");
  import_layout->addWidget(crop_enabled_checkbox_);

  QFormLayout* crop_form = new QFormLayout();
  crop_x_spinbox_ = new QSpinBox();
  crop_x_spinbox_->setRange(0, 10000);
  crop_x_spinbox_->setSuffix(" px");
  crop_form->addRow("X (left):", crop_x_spinbox_);

  crop_y_spinbox_ = new QSpinBox();
  crop_y_spinbox_->setRange(0, 10000);
  crop_y_spinbox_->setSuffix(" px");
  crop_form->addRow("Y (top):", crop_y_spinbox_);

  crop_width_spinbox_ = new QSpinBox();
  crop_width_spinbox_->setRange(0, 10000);
  crop_width_spinbox_->setSuffix(" px (0 = full width)");
  crop_form->addRow("Width:", crop_width_spinbox_);

  crop_height_spinbox_ = new QSpinBox();
  crop_height_spinbox_->setRange(0, 10000);
  crop_height_spinbox_->setSuffix(" px (0 = full height)");
  crop_form->addRow("Height:", crop_height_spinbox_);

  import_layout->addLayout(crop_form);

  // Import Path Configuration
  QLabel* path_label = new QLabel("<b>Import Path Configuration</b>");
  path_label->setContentsMargins(0, 10, 0, 0);
  import_layout->addWidget(path_label);

  QHBoxLayout* base_path_layout = new QHBoxLayout();
  base_path_layout->addWidget(new QLabel("Base Path to Strip:"));
  import_base_path_edit_ = new QLineEdit();
  import_base_path_edit_->setPlaceholderText("/path/to/base/directory");
  browse_import_base_path_button_ = new QPushButton("Browse...");
  base_path_layout->addWidget(import_base_path_edit_, 1);
  base_path_layout->addWidget(browse_import_base_path_button_);
  import_layout->addLayout(base_path_layout);

  QLabel* skip_label = new QLabel("Folders to Skip in Prefix:");
  import_layout->addWidget(skip_label);

  QHBoxLayout* skip_layout = new QHBoxLayout();
  skip_folders_list_ = new QListWidget();
  skip_folders_list_->setMaximumHeight(100);

  QVBoxLayout* skip_btn_layout = new QVBoxLayout();
  add_skip_folder_button_ = new QPushButton("Add");
  remove_skip_folder_button_ = new QPushButton("Remove");
  skip_btn_layout->addWidget(add_skip_folder_button_);
  skip_btn_layout->addWidget(remove_skip_folder_button_);
  skip_btn_layout->addStretch();

  skip_layout->addWidget(skip_folders_list_);
  skip_layout->addLayout(skip_btn_layout);
  import_layout->addLayout(skip_layout);

  // Image Extensions
  QLabel* ext_label = new QLabel("<b>Supported Image Extensions</b>");
  ext_label->setContentsMargins(0, 10, 0, 0);
  import_layout->addWidget(ext_label);

  image_extensions_edit_ = new QLineEdit();
  image_extensions_edit_->setPlaceholderText("jpg, jpeg, png, bmp, tiff");
  import_layout->addWidget(image_extensions_edit_);

  main_layout->addWidget(import_group);

  // ===== Export Settings =====
  QGroupBox* export_group = new QGroupBox("Export Settings");
  QFormLayout* export_layout = new QFormLayout(export_group);

  export_format_combo_ = new QComboBox();
  export_format_combo_->addItem("Segmentation (normalized)");
  export_format_combo_->addItem("Bounding Box (normalized)");
  export_layout->addRow("Export Format:", export_format_combo_);

  normalize_coords_checkbox_ = new QCheckBox("Normalize coordinates (0.0 - 1.0)");
  normalize_coords_checkbox_->setChecked(true);
  export_layout->addRow(normalize_coords_checkbox_);

  QHBoxLayout* precision_layout = new QHBoxLayout();
  coord_precision_spinbox_ = new QSpinBox();
  coord_precision_spinbox_->setRange(0, 10);
  coord_precision_spinbox_->setValue(6);
  coord_precision_spinbox_->setSuffix(" decimal places");
  precision_layout->addWidget(coord_precision_spinbox_);
  precision_layout->addStretch();
  export_layout->addRow("Coordinate Precision:", precision_layout);

  main_layout->addWidget(export_group);
  main_layout->addStretch();
}

void ImportExportSettingsTab::ConnectSignals()
{
  connect(browse_import_base_path_button_, &QPushButton::clicked, this,
          &ImportExportSettingsTab::OnBrowseImportBasePath);

  connect(add_skip_folder_button_, &QPushButton::clicked, this,
          &ImportExportSettingsTab::OnAddSkipFolder);

  connect(remove_skip_folder_button_, &QPushButton::clicked, this,
          &ImportExportSettingsTab::OnRemoveSkipFolder);
}

void ImportExportSettingsTab::LoadFromConfig(const ProjectConfig& config)
{
  // Load export settings (defaults for now)
  export_format_combo_->setCurrentIndex(0);
  normalize_coords_checkbox_->setChecked(true);
  coord_precision_spinbox_->setValue(6);

  // Load crop configuration
  const CropConfig& crop_cfg = config.GetCropConfig();
  crop_enabled_checkbox_->setChecked(crop_cfg.enabled);
  crop_x_spinbox_->setValue(crop_cfg.x);
  crop_y_spinbox_->setValue(crop_cfg.y);
  crop_width_spinbox_->setValue(crop_cfg.width);
  crop_height_spinbox_->setValue(crop_cfg.height);

  // Load import path configuration
  const ImportPathConfig& import_path_cfg = config.GetImportPathConfig();
  import_base_path_edit_->setText(import_path_cfg.base_path);
  skip_folders_list_->clear();
  for (const QString& folder : import_path_cfg.skip_folders)
  {
    skip_folders_list_->addItem(folder);
  }

  // Load image extensions (default for now)
  image_extensions_edit_->setText("jpg, jpeg, png, bmp, tiff");
}

void ImportExportSettingsTab::SaveToConfig(ProjectConfig& config)
{
  // Save crop configuration
  CropConfig crop_cfg;
  crop_cfg.enabled = crop_enabled_checkbox_->isChecked();
  crop_cfg.x = crop_x_spinbox_->value();
  crop_cfg.y = crop_y_spinbox_->value();
  crop_cfg.width = crop_width_spinbox_->value();
  crop_cfg.height = crop_height_spinbox_->value();
  config.SetCropConfig(crop_cfg);

  // Save import path configuration
  ImportPathConfig import_path_cfg;
  import_path_cfg.base_path = import_base_path_edit_->text();
  import_path_cfg.skip_folders.clear();
  for (int i = 0; i < skip_folders_list_->count(); ++i)
  {
    import_path_cfg.skip_folders.append(skip_folders_list_->item(i)->text());
  }
  config.SetImportPathConfig(import_path_cfg);

  // Note: Export settings are currently defaults and not persisted to config
  // Future enhancement: Add export settings to ProjectConfig
}

void ImportExportSettingsTab::OnBrowseImportBasePath()
{
  QString dir = QFileDialog::getExistingDirectory(this, "Select Base Path");
  if (!dir.isEmpty())
  {
    import_base_path_edit_->setText(dir);
  }
}

void ImportExportSettingsTab::OnAddSkipFolder()
{
  bool ok;
  QString folder =
      QInputDialog::getText(this, "Add Folder to Skip", "Folder name:", QLineEdit::Normal, "", &ok);
  if (ok && !folder.isEmpty())
  {
    skip_folders_list_->addItem(folder);
  }
}

void ImportExportSettingsTab::OnRemoveSkipFolder()
{
  QListWidgetItem* item = skip_folders_list_->currentItem();
  if (item)
  {
    delete item;
  }
}

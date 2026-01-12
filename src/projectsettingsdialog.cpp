#include "projectsettingsdialog.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QUrl>
#include <QVBoxLayout>

ProjectSettingsDialog::ProjectSettingsDialog(ProjectConfig& config, QWidget* parent)
    : QDialog(parent), config_(config), original_config_(config)
{
  setWindowTitle("Project Settings");
  setMinimumSize(600, 500);
  SetupUI();
  LoadFromConfig();
}

ProjectSettingsDialog::~ProjectSettingsDialog() {}

void ProjectSettingsDialog::SetupUI()
{
  QVBoxLayout* main_layout = new QVBoxLayout(this);

  // Create tab widget
  tab_widget_ = new QTabWidget(this);

  SetupGeneralTab();
  SetupAIModelTab();
  SetupExportTab();
  SetupAdvancedTab();

  tab_widget_->addTab(general_tab_, "General");
  tab_widget_->addTab(ai_model_tab_, "AI Model");
  tab_widget_->addTab(export_tab_, "Export");
  tab_widget_->addTab(advanced_tab_, "Advanced");

  main_layout->addWidget(tab_widget_);

  // Dialog buttons
  QHBoxLayout* button_layout = new QHBoxLayout();
  button_layout->addStretch();

  apply_button_ = new QPushButton("Apply", this);
  save_button_ = new QPushButton("Save", this);
  cancel_button_ = new QPushButton("Cancel", this);

  button_layout->addWidget(apply_button_);
  button_layout->addWidget(save_button_);
  button_layout->addWidget(cancel_button_);

  main_layout->addLayout(button_layout);

  // Connect buttons
  connect(apply_button_, &QPushButton::clicked, this, &ProjectSettingsDialog::OnApply);
  connect(save_button_, &QPushButton::clicked, this, &ProjectSettingsDialog::OnSave);
  connect(cancel_button_, &QPushButton::clicked, this, &QDialog::reject);
}

void ProjectSettingsDialog::SetupGeneralTab()
{
  general_tab_ = new QWidget();
  QFormLayout* layout = new QFormLayout(general_tab_);

  // Project name
  project_name_edit_ = new QLineEdit();
  layout->addRow("Project Name:", project_name_edit_);

  // Auto-save settings
  QGroupBox* auto_save_group = new QGroupBox("Auto-Save Settings");
  QVBoxLayout* auto_save_layout = new QVBoxLayout(auto_save_group);

  auto_save_checkbox_ = new QCheckBox("Enable auto-save");
  auto_save_checkbox_->setChecked(true);
  auto_save_layout->addWidget(auto_save_checkbox_);

  QHBoxLayout* interval_layout = new QHBoxLayout();
  QLabel* interval_label = new QLabel("Auto-save interval:");
  auto_save_interval_spinbox_ = new QSpinBox();
  auto_save_interval_spinbox_->setRange(10, 300);
  auto_save_interval_spinbox_->setSuffix(" seconds");
  auto_save_interval_spinbox_->setValue(30);

  interval_layout->addWidget(interval_label);
  interval_layout->addWidget(auto_save_interval_spinbox_);
  interval_layout->addStretch();

  auto_save_layout->addLayout(interval_layout);

  layout->addRow(auto_save_group);
  layout->addRow(new QWidget());  // Spacer
}

void ProjectSettingsDialog::SetupAIModelTab()
{
  ai_model_tab_ = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout(ai_model_tab_);

  // Model path
  QGroupBox* model_group = new QGroupBox("Model Configuration");
  QVBoxLayout* model_layout = new QVBoxLayout(model_group);

  QHBoxLayout* path_layout = new QHBoxLayout();
  QLabel* path_label = new QLabel("Model Path:");
  model_path_edit_ = new QLineEdit();
  model_path_edit_->setPlaceholderText("./models/model.pt");
  browse_model_button_ = new QPushButton("Browse...");

  path_layout->addWidget(path_label);
  path_layout->addWidget(model_path_edit_, 1);
  path_layout->addWidget(browse_model_button_);

  model_layout->addLayout(path_layout);

  // Confidence threshold
  QHBoxLayout* conf_layout = new QHBoxLayout();
  QLabel* conf_label = new QLabel("Confidence Threshold:");
  confidence_slider_ = new QSlider(Qt::Horizontal);
  confidence_slider_->setRange(25, 75);
  confidence_slider_->setValue(25);
  confidence_value_label_ = new QLabel("0.25");

  conf_layout->addWidget(conf_label);
  conf_layout->addWidget(confidence_slider_, 1);
  conf_layout->addWidget(confidence_value_label_);

  model_layout->addLayout(conf_layout);

  // Model info
  model_info_label_ = new QLabel("No model loaded");
  model_info_label_->setStyleSheet(
      "QLabel { padding: 10px; background-color: #f0f0f0; border-radius: 5px; }");
  model_layout->addWidget(model_info_label_);

  layout->addWidget(model_group);

  // Download button
  QHBoxLayout* download_layout = new QHBoxLayout();
  download_layout->addStretch();
  download_model_button_ = new QPushButton("Download Models from Ultralytics");
  download_layout->addWidget(download_model_button_);
  download_layout->addStretch();

  layout->addLayout(download_layout);
  layout->addStretch();

  // Connect signals
  connect(browse_model_button_, &QPushButton::clicked, this,
          [this]()
          {
            QString file =
                QFileDialog::getOpenFileName(this, "Select Model", model_path_edit_->text(),
                                             "Model Files (*.pt *.onnx *.pth);;All Files (*)");
            if (!file.isEmpty())
            {
              model_path_edit_->setText(file);
            }
          });

  connect(confidence_slider_, &QSlider::valueChanged, this, [this](int value)
          { confidence_value_label_->setText(QString::number(value / 100.0, 'f', 2)); });

  connect(download_model_button_, &QPushButton::clicked, this,
          []() { QDesktopServices::openUrl(QUrl("https://docs.ultralytics.com/models/")); });
}

void ProjectSettingsDialog::SetupExportTab()
{
  export_tab_ = new QWidget();
  QFormLayout* layout = new QFormLayout(export_tab_);

  // Export format
  export_format_combo_ = new QComboBox();
  export_format_combo_->addItem("Segmentation (normalized)");
  export_format_combo_->addItem("Bounding Box (normalized)");
  export_format_combo_->addItem("COCO JSON");
  layout->addRow("Export Format:", export_format_combo_);

  // Normalize coordinates
  normalize_coords_checkbox_ = new QCheckBox("Normalize coordinates (0.0 - 1.0)");
  normalize_coords_checkbox_->setChecked(true);
  layout->addRow(normalize_coords_checkbox_);

  // Coordinate precision
  QHBoxLayout* precision_layout = new QHBoxLayout();
  QLabel* precision_label = new QLabel("Coordinate Precision:");
  coord_precision_spinbox_ = new QSpinBox();
  coord_precision_spinbox_->setRange(0, 10);
  coord_precision_spinbox_->setValue(6);
  coord_precision_spinbox_->setSuffix(" decimal places");

  precision_layout->addWidget(precision_label);
  precision_layout->addWidget(coord_precision_spinbox_);
  precision_layout->addStretch();

  layout->addRow(precision_layout);
  layout->addRow(new QWidget());  // Spacer
}

void ProjectSettingsDialog::SetupAdvancedTab()
{
  advanced_tab_ = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout(advanced_tab_);

  // Image extensions
  QGroupBox* extensions_group = new QGroupBox("Supported Image Extensions");
  QVBoxLayout* ext_layout = new QVBoxLayout(extensions_group);
  image_extensions_edit_ = new QLineEdit();
  image_extensions_edit_->setPlaceholderText("jpg, jpeg, png, bmp, tiff");
  ext_layout->addWidget(image_extensions_edit_);
  layout->addWidget(extensions_group);

  // Custom folder paths
  QGroupBox* paths_group = new QGroupBox("Custom Folder Paths");
  QFormLayout* paths_layout = new QFormLayout(paths_group);

  // Images folder
  QHBoxLayout* images_layout = new QHBoxLayout();
  images_folder_edit_ = new QLineEdit();
  images_folder_edit_->setPlaceholderText("images");
  browse_images_button_ = new QPushButton("Browse...");
  images_layout->addWidget(images_folder_edit_, 1);
  images_layout->addWidget(browse_images_button_);
  paths_layout->addRow("Images Folder:", images_layout);

  // Labels folder
  QHBoxLayout* labels_layout = new QHBoxLayout();
  labels_folder_edit_ = new QLineEdit();
  labels_folder_edit_->setPlaceholderText("labels");
  browse_labels_button_ = new QPushButton("Browse...");
  labels_layout->addWidget(labels_folder_edit_, 1);
  labels_layout->addWidget(browse_labels_button_);
  paths_layout->addRow("Labels Folder:", labels_layout);

  // Models folder
  QHBoxLayout* models_layout = new QHBoxLayout();
  models_folder_edit_ = new QLineEdit();
  models_folder_edit_->setPlaceholderText("models");
  browse_models_button_ = new QPushButton("Browse...");
  models_layout->addWidget(models_folder_edit_, 1);
  models_layout->addWidget(browse_models_button_);
  paths_layout->addRow("Models Folder:", models_layout);

  layout->addWidget(paths_group);
  layout->addStretch();

  // Connect browse buttons
  connect(browse_images_button_, &QPushButton::clicked, this,
          [this]()
          {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Images Folder");
            if (!dir.isEmpty())
            {
              images_folder_edit_->setText(dir);
            }
          });

  connect(browse_labels_button_, &QPushButton::clicked, this,
          [this]()
          {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Labels Folder");
            if (!dir.isEmpty())
            {
              labels_folder_edit_->setText(dir);
            }
          });

  connect(browse_models_button_, &QPushButton::clicked, this,
          [this]()
          {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Models Folder");
            if (!dir.isEmpty())
            {
              models_folder_edit_->setText(dir);
            }
          });
}

void ProjectSettingsDialog::LoadFromConfig()
{
  // General tab
  project_name_edit_->setText(config_.GetProjectName());
  // Auto-save settings would come from config if we add them

  // AI Model tab - from plugin settings
  const PluginConfig& plugin = config_.GetPluginConfig();
  if (plugin.enabled)
  {
    QString model_path = plugin.settings.value("model", "");
    model_path_edit_->setText(model_path);

    QString conf = plugin.settings.value("confidence", "0.25");
    int conf_value = static_cast<int>(conf.toDouble() * 100);
    confidence_slider_->setValue(conf_value);
  }

  // Export tab - defaults for now
  export_format_combo_->setCurrentIndex(0);
  normalize_coords_checkbox_->setChecked(true);
  coord_precision_spinbox_->setValue(6);

  // Advanced tab - defaults
  image_extensions_edit_->setText("jpg, jpeg, png, bmp, tiff");
  images_folder_edit_->setText("images");
  labels_folder_edit_->setText("labels");
  models_folder_edit_->setText("models");
}

void ProjectSettingsDialog::SaveToConfig()
{
  // General tab
  config_.SetProjectName(project_name_edit_->text());

  // AI Model tab - save to plugin settings
  PluginConfig plugin = config_.GetPluginConfig();
  if (plugin.enabled)
  {
    plugin.settings["model"] = model_path_edit_->text();
    plugin.settings["confidence"] = QString::number(confidence_slider_->value() / 100.0, 'f', 2);
    config_.SetPluginConfig(plugin);
  }

  // Export and Advanced settings would be saved here
  // For now, we're focusing on the UI structure
}

void ProjectSettingsDialog::OnApply()
{
  SaveToConfig();
  QMessageBox::information(this, "Settings Applied",
                           "Settings have been applied to the current session.");
}

void ProjectSettingsDialog::OnSave()
{
  SaveToConfig();
  accept();
}

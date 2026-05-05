#include "metadataimportsettingsdialog.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>

#include "ui_metadataimportsettingsdialog.h"

MetadataImportSettingsDialog::MetadataImportSettingsDialog(const QString& filepath,
                                                         int width, int height,
                                                         QWidget* parent)
    : QDialog(parent),
      ui_(new Ui::MetadataImportSettingsDialog),
      filepath_(filepath),
      file_width_(width),
      file_height_(height)
{
  ui_->setupUi(this);

  // Initialize controls with file information and default values
  InitializeControls();

  // Set up signal connections
  connect(ui_->crop_enabled_checkbox_, &QCheckBox::toggled,
          this, &MetadataImportSettingsDialog::OnCropEnabledChanged);
  connect(ui_->import_button_, &QPushButton::clicked,
          this, &MetadataImportSettingsDialog::ValidateSettings);

  // Update crop control limits
  UpdateCropLimits();

  // Initially disable crop controls if cropping is not enabled
  OnCropEnabledChanged(ui_->crop_enabled_checkbox_->isChecked());
}

MetadataImportSettingsDialog::~MetadataImportSettingsDialog()
{
  delete ui_;
}

void MetadataImportSettingsDialog::InitializeControls()
{
  // Set file information display
  QFileInfo file_info(filepath_);
  ui_->file_path_value_->setText(file_info.fileName());
  ui_->file_path_value_->setToolTip(filepath_);  // Show full path on hover
  ui_->dimensions_value_->setText(QString("%1 x %2").arg(file_width_).arg(file_height_));

  // Set default range values
  ui_->range_min_spinbox_->setValue(0.0);
  ui_->range_max_spinbox_->setValue(100.0);

  // Set default out-of-range handling (clamp is already checked by default in UI)
  ui_->clamp_radio_->setChecked(true);

  // Set default crop settings
  ui_->crop_enabled_checkbox_->setChecked(false);
  ui_->crop_start_x_spinbox_->setValue(0);
  ui_->crop_start_y_spinbox_->setValue(0);
  ui_->crop_end_x_spinbox_->setValue(file_width_);
  ui_->crop_end_y_spinbox_->setValue(file_height_);
}

void MetadataImportSettingsDialog::UpdateCropLimits()
{
  // Update maximum values for crop controls based on file dimensions
  ui_->crop_start_x_spinbox_->setMaximum(file_width_ - 1);
  ui_->crop_start_y_spinbox_->setMaximum(file_height_ - 1);
  ui_->crop_end_x_spinbox_->setMaximum(file_width_);
  ui_->crop_end_y_spinbox_->setMaximum(file_height_);

  // Set default end values to full dimensions
  ui_->crop_end_x_spinbox_->setValue(file_width_);
  ui_->crop_end_y_spinbox_->setValue(file_height_);
}

void MetadataImportSettingsDialog::OnCropEnabledChanged(bool enabled)
{
  // Enable/disable crop controls based on checkbox state
  ui_->crop_start_x_spinbox_->setEnabled(enabled);
  ui_->crop_start_y_spinbox_->setEnabled(enabled);
  ui_->crop_end_x_spinbox_->setEnabled(enabled);
  ui_->crop_end_y_spinbox_->setEnabled(enabled);
  ui_->crop_start_x_label_->setEnabled(enabled);
  ui_->crop_start_y_label_->setEnabled(enabled);
  ui_->crop_end_x_label_->setEnabled(enabled);
  ui_->crop_end_y_label_->setEnabled(enabled);
  ui_->crop_start_label_->setEnabled(enabled);
  ui_->crop_end_label_->setEnabled(enabled);
}

void MetadataImportSettingsDialog::ValidateSettings()
{
  // Validate range settings
  double range_min = ui_->range_min_spinbox_->value();
  double range_max = ui_->range_max_spinbox_->value();

  if (range_min >= range_max)
  {
    QMessageBox::warning(this, "Invalid Range",
                        "Maximum value must be greater than minimum value.\n\n"
                        "Please adjust the range values.");
    ui_->range_max_spinbox_->setFocus();
    ui_->range_max_spinbox_->selectAll();
    return;
  }

  // Validate crop settings if enabled
  if (ui_->crop_enabled_checkbox_->isChecked())
  {
    int start_x = ui_->crop_start_x_spinbox_->value();
    int start_y = ui_->crop_start_y_spinbox_->value();
    int end_x = ui_->crop_end_x_spinbox_->value();
    int end_y = ui_->crop_end_y_spinbox_->value();

    if (start_x >= end_x || start_y >= end_y)
    {
      QMessageBox::warning(this, "Invalid Crop Region",
                          "End coordinates must be greater than start coordinates.\n\n"
                          "Please adjust the crop region.");
      return;
    }

    if (end_x > file_width_ || end_y > file_height_)
    {
      QMessageBox::warning(this, "Invalid Crop Region",
                          QString("Crop region extends outside data boundaries.\n\n"
                                  "Data size: %1 x %2\n"
                                  "Crop region: (%3,%4) to (%5,%6)")
                                  .arg(file_width_).arg(file_height_)
                                  .arg(start_x).arg(start_y)
                                  .arg(end_x).arg(end_y));
      return;
    }

    // Check for minimum crop size
    if ((end_x - start_x) < 2 || (end_y - start_y) < 2)
    {
      QMessageBox::warning(this, "Invalid Crop Region",
                          "Crop region must be at least 2x2 pixels.\n\n"
                          "Please adjust the crop region.");
      return;
    }
  }

  // All validations passed, accept the dialog
  accept();
}

MetadataImporter::ImportSettings MetadataImportSettingsDialog::GetSettings() const
{
  MetadataImporter::ImportSettings settings;

  // Range settings
  settings.range_min = ui_->range_min_spinbox_->value();
  settings.range_max = ui_->range_max_spinbox_->value();

  // Out-of-range handling
  if (ui_->clamp_radio_->isChecked())
  {
    settings.out_of_range_handling = MetadataImporter::ImportSettings::CLAMP_TO_BOUNDS;
  }
  else if (ui_->zero_radio_->isChecked())
  {
    settings.out_of_range_handling = MetadataImporter::ImportSettings::SET_TO_ZERO;
  }
  else if (ui_->max_radio_->isChecked())
  {
    settings.out_of_range_handling = MetadataImporter::ImportSettings::SET_TO_MAX;
  }
  else
  {
    // Default fallback
    settings.out_of_range_handling = MetadataImporter::ImportSettings::CLAMP_TO_BOUNDS;
  }

  // Crop settings
  settings.enable_cropping = ui_->crop_enabled_checkbox_->isChecked();
  if (settings.enable_cropping)
  {
    settings.crop_start_x = ui_->crop_start_x_spinbox_->value();
    settings.crop_start_y = ui_->crop_start_y_spinbox_->value();
    settings.crop_end_x = ui_->crop_end_x_spinbox_->value();
    settings.crop_end_y = ui_->crop_end_y_spinbox_->value();
  }
  else
  {
    // Set to full dimensions when cropping is disabled
    settings.crop_start_x = 0;
    settings.crop_start_y = 0;
    settings.crop_end_x = file_width_;
    settings.crop_end_y = file_height_;
  }

  return settings;
}
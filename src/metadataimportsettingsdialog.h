#ifndef METADATAIMPORTSETTINGSDIALOG_H
#define METADATAIMPORTSETTINGSDIALOG_H

#include <QDialog>
#include "metadataimporter.h"

namespace Ui
{
class MetadataImportSettingsDialog;
}

/**
 * @brief Dialog for configuring metadata import settings
 *
 * Allows users to configure data range, out-of-range handling, and
 * optional crop region before importing numerical metadata files
 * as grayscale images.
 */
class MetadataImportSettingsDialog : public QDialog
{
  Q_OBJECT

 public:
  explicit MetadataImportSettingsDialog(const QString& filepath,
                                       int width, int height,
                                       QWidget* parent = nullptr);
  ~MetadataImportSettingsDialog();

  /**
   * @brief Get configured import settings
   * @return ImportSettings configured by user
   */
  MetadataImporter::ImportSettings GetSettings() const;

 private slots:
  /**
   * @brief Handle crop enabled/disabled state change
   * @param enabled Whether cropping is enabled
   */
  void OnCropEnabledChanged(bool enabled);

  /**
   * @brief Validate user settings before accepting dialog
   */
  void ValidateSettings();

 private:
  /**
   * @brief Initialize UI controls with default values
   */
  void InitializeControls();

  /**
   * @brief Update crop control maximum values based on file dimensions
   */
  void UpdateCropLimits();

  Ui::MetadataImportSettingsDialog* ui_;
  QString filepath_;
  int file_width_;
  int file_height_;
};

#endif  // METADATAIMPORTSETTINGSDIALOG_H
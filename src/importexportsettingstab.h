#ifndef IMPORTEXPORTSETTINGSTAB_H
#define IMPORTEXPORTSETTINGSTAB_H

#include "settingstabbase.h"

class ProjectConfig;

namespace Ui
{
class ImportExportSettingsTab;
}

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
  ~ImportExportSettingsTab() override;

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
  Ui::ImportExportSettingsTab* ui_;
  ProjectConfig& config_;
};

#endif  // IMPORTEXPORTSETTINGSTAB_H

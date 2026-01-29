#ifndef PROJECTSETTINGSTAB_H
#define PROJECTSETTINGSTAB_H

#include "settingstabbase.h"

class ProjectConfig;

namespace Ui
{
class ProjectSettingsTab;
}

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
  ~ProjectSettingsTab() override;

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
  Ui::ProjectSettingsTab* ui_;
  ProjectConfig& config_;
  QString project_dir_;
};

#endif  // PROJECTSETTINGSTAB_H

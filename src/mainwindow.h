#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "projectconfig.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
class QLabel;
QT_END_NAMESPACE

class AIPluginManager;

class MainWindow : public QMainWindow
{
  Q_OBJECT

 public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

 public slots:
  void Load();
  void Save();

  void Increase();
  void Decrease();
  void ResetZoom();

  void OnClassSelected(int index);
  void NextClass();
  void PreviousClass();
  void SelectClassByNumber(int number);
  void AddImagesToProject();
  void ImportDataAsImage();

  // Project Management
  void CreateNewProject();
  void OpenProject();
  void OpenRecentProject();
  void SaveProject();

  // Image Navigation
  void LoadImageAtIndex(int index);
  void AutoSaveCurrentImage();
  void NextImage();
  void PreviousImage();
  void FirstImage();
  void LastImage();

  // AI Plugin Interface
  void RunAutoDetect();
  void RunBatchDetect();
  void RunTrainModel();
  void ShowPluginWizard();

  // Meta file review
  void ApproveCurrentAnnotations();
  void RejectCurrentAnnotations();
  void NextUnreviewedImage();

  // Settings
  void ShowProjectSettings();
  void PromptModelRegistration();
  void RegisterModelManually();

  // Statistics
  void ShowProjectStatistics();

  // Edit operations
  void Undo();
  void Redo();
  void CopyPolygon();
  void PastePolygon();

  // Help
  void ShowKeyboardShortcuts();
  void ShowAboutDialog();
  void EditShortcuts();

 protected:
  void keyPressEvent(QKeyEvent* event) override;

 private:
  void SaveProjectConfig();
  void UpdateWindowTitle();
  void ScanProjectImages();
  void UpdateStatusBar();
  void LoadShortcuts();
  void SaveShortcuts();
  void ApplyShortcuts();
  void AddToRecentProjects(const QString& projectPath);
  void UpdateRecentProjectsMenu();
  void LoadLastProject();
  QString GetProjectStatistics() const;

  Ui::MainWindow* ui;
  QString current_image_path_;
  QString project_file_path_;
  QString project_directory_;
  QStringList image_list_;
  int current_image_index_;
  ProjectConfig project_config_;
  int current_class_id_;

  // Status bar labels
  QLabel* status_left_;
  QLabel* status_center_;
  QLabel* status_right_;

  // Keyboard shortcuts
  QMap<QString, QString> shortcuts_;
  
  // Recent projects
  QMenu* recent_projects_menu_;

  // AI Plugin Manager
  AIPluginManager* ai_plugin_manager_;
};
#endif  // MAINWINDOW_H

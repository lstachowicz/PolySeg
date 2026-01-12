#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include "projectconfig.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QLabel;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void Load();
    void Save();

    void Increase();
    void Decrease();
    void ResetZoom();
    
    void OnClassSelected(int index);
    void ManageClasses();
    void OpenFolder();
    
    // Project Management
    void CreateNewProject();
    void OpenProject();
    void SaveProject();
    
    // Image Navigation
    void LoadImageAtIndex(int index);
    void AutoSaveCurrentImage();
    void NextImage();
    void PreviousImage();
    void FirstImage();
    void LastImage();
    
    // AI Plugin Interface
    void ConfigurePlugin();
    void RunAutoDetect();
    void RunBatchDetect();
    void RunTrainModel();
    
    // Meta file review
    void ApproveCurrentAnnotations();
    void RejectCurrentAnnotations();
    void NextUnreviewedImage();
    
    // Settings
    void ShowProjectSettings();
    
    // Statistics
    void ShowProjectStatistics();

private:
    void UpdateClassComboBox();
    void SaveProjectConfig();
    void UpdateWindowTitle();
    void ScanProjectImages();
    void UpdateStatusBar();
    QString GetProjectStatistics() const;
    
    // Plugin helpers
    bool IsPluginAvailable() const;
    QString BuildPluginCommand(const QString& args_template, const QMap<QString, QString>& variables) const;
    void ExecutePluginCommand(const QString& command, const QStringList& args);
    void ParseDetectionResults(const QString& json_output);
    
    // Batch detection & meta file management
    void BatchDetectOnImage(const QString& image_path);
    void SaveToMetaFile(const QString& image_path);
    void LoadFromMetaFile(const QString& image_path);
    bool HasMetaFile(const QString& image_path) const;
    bool HasApprovedFile(const QString& image_path) const;
    void PromoteMetaToApproved(const QString& image_path);
    void DeleteMetaFile(const QString& image_path);
    int CountUnreviewedImages() const;
    
    Ui::MainWindow *ui;
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
};
#endif // MAINWINDOW_H

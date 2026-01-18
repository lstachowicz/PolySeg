#ifndef AIPLUGINMANAGER_H
#define AIPLUGINMANAGER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class ProjectConfig;
class PolygonCanvas;
class QStatusBar;

class AIPluginManager : public QObject
{
  Q_OBJECT

 public:
  explicit AIPluginManager(QObject* parent = nullptr);
  ~AIPluginManager();

  // Dependency injection
  void SetProjectConfig(ProjectConfig* config);
  void SetCanvas(PolygonCanvas* canvas);
  void SetStatusBar(QStatusBar* status_bar);
  void SetProjectDirectory(const QString& dir);
  void SetImageList(const QStringList* list);

  // Plugin availability
  bool IsPluginAvailable() const;

  // Single image detection
  void RunAutoDetect(const QString& current_image_path);

  // Batch detection
  void RunBatchDetect();
  void BatchDetectOnImage(const QString& image_path);

  // Training
  void RunTrainModel();

  // Model registration
  void PromptModelRegistration();
  void RegisterModelManually();

  // Meta file operations
  void SaveToMetaFile(const QString& image_path);
  void LoadFromMetaFile(const QString& image_path);
  bool HasMetaFile(const QString& image_path) const;
  bool HasApprovedFile(const QString& image_path) const;
  void PromoteMetaToApproved(const QString& image_path);
  void DeleteMetaFile(const QString& image_path);
  int CountUnreviewedImages() const;

 signals:
  void DetectionComplete(int count);
  void TrainingComplete(bool success);
  void StatusMessage(const QString& message, int timeout = 0);
  void RequestNextUnreviewed();
  void ClassesUpdated();

 private:
  QString BuildPluginCommand(const QString& args_template,
                             const QMap<QString, QString>& variables) const;
  void ExecutePluginCommand(const QString& command, const QStringList& args);
  void ParseDetectionResults(const QString& json_output);

  ProjectConfig* project_config_;
  PolygonCanvas* canvas_;
  QStatusBar* status_bar_;
  QString project_directory_;
  const QStringList* image_list_;
  QProcess* training_process_;
};

#endif  // AIPLUGINMANAGER_H

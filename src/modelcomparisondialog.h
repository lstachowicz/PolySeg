#ifndef MODELCOMPARISONDIALOG_H
#define MODELCOMPARISONDIALOG_H

#include <QDialog>

#include "projectconfig.h"

class PolygonCanvas;

namespace Ui
{
class ModelComparisonDialog;
}

class ModelComparisonDialog : public QDialog
{
  Q_OBJECT

 public:
  explicit ModelComparisonDialog(ProjectConfig& config, const QString& project_dir,
                                 QWidget* parent = nullptr);
  ~ModelComparisonDialog();

 private slots:
  void OnModelAChanged(int index);
  void OnModelBChanged(int index);
  void OnPreviousImage();
  void OnNextImage();
  void OnImageSelected(int index);
  void RunComparison();

 private:
  void SetupUI();
  void ConnectSignals();
  void LoadTestImages();
  void LoadImageAtIndex(int index);
  void RunDetectionOnModel(const QString& model_path, PolygonCanvas* canvas);
  QString GetModelPath(int model_index) const;

  Ui::ModelComparisonDialog* ui_;
  ProjectConfig& config_;
  QString project_dir_;
  QStringList test_images_;
  int current_image_index_;
};

#endif  // MODELCOMPARISONDIALOG_H

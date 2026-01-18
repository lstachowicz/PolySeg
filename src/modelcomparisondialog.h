#ifndef MODELCOMPARISONDIALOG_H
#define MODELCOMPARISONDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>

#include "projectconfig.h"

class PolygonCanvas;

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
  void LoadTestImages();
  void LoadImageAtIndex(int index);
  void RunDetectionOnModel(const QString& model_path, PolygonCanvas* canvas);
  QString GetModelPath(int model_index) const;

  ProjectConfig& config_;
  QString project_dir_;
  QStringList test_images_;
  int current_image_index_;

  // UI components
  QComboBox* model_a_combo_;
  QComboBox* model_b_combo_;
  QComboBox* image_combo_;
  QPushButton* prev_button_;
  QPushButton* next_button_;
  QPushButton* compare_button_;

  PolygonCanvas* canvas_a_;
  PolygonCanvas* canvas_b_;
  QLabel* stats_a_;
  QLabel* stats_b_;
};

#endif  // MODELCOMPARISONDIALOG_H

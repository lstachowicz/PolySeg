#include "modelcomparisondialog.h"

#include <QMessageBox>

#include "polygoncanvas.h"
#include "ui_modelcomparisondialog.h"

ModelComparisonDialog::ModelComparisonDialog(ProjectConfig& config, const QString& project_dir,
                                             QWidget* parent)
    : QDialog(parent),
      ui_(new Ui::ModelComparisonDialog),
      config_(config),
      project_dir_(project_dir),
      current_image_index_(0)
{
  ui_->setupUi(this);
  SetupUI();
  ConnectSignals();
  LoadTestImages();
}

ModelComparisonDialog::~ModelComparisonDialog()
{
  delete ui_;
}

void ModelComparisonDialog::SetupUI()
{
  // Populate model dropdowns
  const QList<ModelVersion>& models = config_.GetModelVersions();
  for (const ModelVersion& model : models)
  {
    QString display = QString("%1 (%2 imgs)").arg(model.name).arg(model.training_images_count);
    ui_->model_a_combo_->addItem(display);
    ui_->model_b_combo_->addItem(display);
  }

  // Set default selection if we have at least 2 models
  if (models.size() >= 2)
  {
    ui_->model_a_combo_->setCurrentIndex(0);
    ui_->model_b_combo_->setCurrentIndex(1);
  }
}

void ModelComparisonDialog::ConnectSignals()
{
  connect(ui_->model_a_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelComparisonDialog::OnModelAChanged);
  connect(ui_->model_b_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelComparisonDialog::OnModelBChanged);
  connect(ui_->image_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelComparisonDialog::OnImageSelected);
  connect(ui_->prev_button_, &QPushButton::clicked, this, &ModelComparisonDialog::OnPreviousImage);
  connect(ui_->next_button_, &QPushButton::clicked, this, &ModelComparisonDialog::OnNextImage);
  connect(ui_->compare_button_, &QPushButton::clicked, this, &ModelComparisonDialog::RunComparison);
}

void ModelComparisonDialog::LoadTestImages()
{
  // Get all images assigned to test split
  test_images_.clear();
  const QMap<QString, QString>& splits = config_.GetImageSplits();

  for (auto it = splits.begin(); it != splits.end(); ++it)
  {
    if (it.value() == "test")
    {
      test_images_.append(it.key());
    }
  }

  // Populate image dropdown
  ui_->image_combo_->clear();
  for (const QString& img : test_images_)
  {
    ui_->image_combo_->addItem(img);
  }

  if (test_images_.isEmpty())
  {
    QMessageBox::warning(this, "No Test Images",
                         "No images are assigned to the test split.\n\n"
                         "Please configure dataset splits in Project Settings.");
    ui_->compare_button_->setEnabled(false);
  }
  else
  {
    LoadImageAtIndex(0);
  }
}

void ModelComparisonDialog::LoadImageAtIndex(int index)
{
  if (index < 0 || index >= test_images_.size())
  {
    return;
  }

  current_image_index_ = index;
  QString image_path = project_dir_ + "/images/" + test_images_[index];

  // Load image in both canvases
  QPixmap pixmap(image_path);
  if (!pixmap.isNull())
  {
    ui_->canvas_a_->setPixmap(pixmap);
    ui_->canvas_b_->setPixmap(pixmap);
    ui_->canvas_a_->ClearAllPolygons();
    ui_->canvas_b_->ClearAllPolygons();

    // Reset stats
    ui_->stats_a_->setText("Not yet run");
    ui_->stats_b_->setText("Not yet run");
  }

  // Update combo box
  ui_->image_combo_->setCurrentIndex(index);

  // Update navigation buttons
  ui_->prev_button_->setEnabled(index > 0);
  ui_->next_button_->setEnabled(index < test_images_.size() - 1);
}

void ModelComparisonDialog::RunDetectionOnModel(const QString& model_path, PolygonCanvas* canvas)
{
  // TODO: This should call the plugin with the specific model
  // For now, just show a placeholder
  Q_UNUSED(model_path);
  Q_UNUSED(canvas);

  QMessageBox::information(
      this, "Detection Placeholder",
      QString("Would run detection using model:\n%1\n\nThis requires plugin integration.")
          .arg(model_path));
}

void ModelComparisonDialog::RunComparison()
{
  if (test_images_.isEmpty())
  {
    return;
  }

  const QList<ModelVersion>& models = config_.GetModelVersions();

  int model_a_idx = ui_->model_a_combo_->currentIndex();
  int model_b_idx = ui_->model_b_combo_->currentIndex();

  if (model_a_idx < 0 || model_a_idx >= models.size() || model_b_idx < 0 ||
      model_b_idx >= models.size())
  {
    QMessageBox::warning(this, "Invalid Selection", "Please select both models.");
    return;
  }

  QString model_a_path = project_dir_ + "/" + models[model_a_idx].path;
  QString model_b_path = project_dir_ + "/" + models[model_b_idx].path;

  // Clear previous results
  ui_->canvas_a_->ClearAllPolygons();
  ui_->canvas_b_->ClearAllPolygons();

  // Run detection on both models
  RunDetectionOnModel(model_a_path, ui_->canvas_a_);
  RunDetectionOnModel(model_b_path, ui_->canvas_b_);

  // Update stats (placeholder for now)
  int count_a = ui_->canvas_a_->GetPolygons().size();
  int count_b = ui_->canvas_b_->GetPolygons().size();

  ui_->stats_a_->setText(QString("Detections: %1").arg(count_a));
  ui_->stats_b_->setText(QString("Detections: %1").arg(count_b));
}

QString ModelComparisonDialog::GetModelPath(int model_index) const
{
  const QList<ModelVersion>& models = config_.GetModelVersions();
  if (model_index >= 0 && model_index < models.size())
  {
    return project_dir_ + "/" + models[model_index].path;
  }
  return QString();
}

void ModelComparisonDialog::OnModelAChanged(int index)
{
  Q_UNUSED(index);
  // Reset results when model changes
  ui_->canvas_a_->ClearAllPolygons();
  ui_->stats_a_->setText("Not yet run");
}

void ModelComparisonDialog::OnModelBChanged(int index)
{
  Q_UNUSED(index);
  // Reset results when model changes
  ui_->canvas_b_->ClearAllPolygons();
  ui_->stats_b_->setText("Not yet run");
}

void ModelComparisonDialog::OnPreviousImage()
{
  if (current_image_index_ > 0)
  {
    LoadImageAtIndex(current_image_index_ - 1);
  }
}

void ModelComparisonDialog::OnNextImage()
{
  if (current_image_index_ < test_images_.size() - 1)
  {
    LoadImageAtIndex(current_image_index_ + 1);
  }
}

void ModelComparisonDialog::OnImageSelected(int index)
{
  LoadImageAtIndex(index);
}

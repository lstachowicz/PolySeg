#include "modelcomparisondialog.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include "polygoncanvas.h"

ModelComparisonDialog::ModelComparisonDialog(ProjectConfig& config, const QString& project_dir,
                                             QWidget* parent)
    : QDialog(parent), config_(config), project_dir_(project_dir), current_image_index_(0)
{
  setWindowTitle("Compare Model Versions");
  setMinimumSize(1200, 700);
  SetupUI();
  LoadTestImages();
}

ModelComparisonDialog::~ModelComparisonDialog() {}

void ModelComparisonDialog::SetupUI()
{
  QVBoxLayout* main_layout = new QVBoxLayout(this);

  // Top controls
  QHBoxLayout* controls_layout = new QHBoxLayout();

  // Model A selector
  controls_layout->addWidget(new QLabel("Model A:"));
  model_a_combo_ = new QComboBox();
  controls_layout->addWidget(model_a_combo_);

  controls_layout->addSpacing(20);

  // Model B selector
  controls_layout->addWidget(new QLabel("Model B:"));
  model_b_combo_ = new QComboBox();
  controls_layout->addWidget(model_b_combo_);

  controls_layout->addSpacing(20);

  // Image selector
  controls_layout->addWidget(new QLabel("Test Image:"));
  image_combo_ = new QComboBox();
  image_combo_->setMinimumWidth(200);
  controls_layout->addWidget(image_combo_);

  // Navigation buttons
  prev_button_ = new QPushButton("Previous");
  next_button_ = new QPushButton("Next");
  controls_layout->addWidget(prev_button_);
  controls_layout->addWidget(next_button_);

  controls_layout->addStretch();

  // Compare button
  compare_button_ = new QPushButton("Run Comparison");
  compare_button_->setStyleSheet("QPushButton { font-weight: bold; }");
  controls_layout->addWidget(compare_button_);

  main_layout->addLayout(controls_layout);

  // Comparison view - dual panes
  QHBoxLayout* comparison_layout = new QHBoxLayout();

  // Left pane - Model A
  QGroupBox* pane_a = new QGroupBox("Model A Results");
  QVBoxLayout* layout_a = new QVBoxLayout(pane_a);
  canvas_a_ = new PolygonCanvas();
  canvas_a_->setMinimumSize(500, 400);
  layout_a->addWidget(canvas_a_);
  stats_a_ = new QLabel("Not yet run");
  stats_a_->setStyleSheet(
      "QLabel { padding: 5px; background-color: #f0f0f0; border-radius: 3px; }");
  layout_a->addWidget(stats_a_);
  comparison_layout->addWidget(pane_a);

  // Right pane - Model B
  QGroupBox* pane_b = new QGroupBox("Model B Results");
  QVBoxLayout* layout_b = new QVBoxLayout(pane_b);
  canvas_b_ = new PolygonCanvas();
  canvas_b_->setMinimumSize(500, 400);
  layout_b->addWidget(canvas_b_);
  stats_b_ = new QLabel("Not yet run");
  stats_b_->setStyleSheet(
      "QLabel { padding: 5px; background-color: #f0f0f0; border-radius: 3px; }");
  layout_b->addWidget(stats_b_);
  comparison_layout->addWidget(pane_b);

  main_layout->addLayout(comparison_layout);

  // Populate model dropdowns
  const QList<ModelVersion>& models = config_.GetModelVersions();
  for (const ModelVersion& model : models)
  {
    QString display = QString("%1 (%2 imgs)").arg(model.name).arg(model.training_images_count);
    model_a_combo_->addItem(display);
    model_b_combo_->addItem(display);
  }

  // Set default selection if we have at least 2 models
  if (models.size() >= 2)
  {
    model_a_combo_->setCurrentIndex(0);
    model_b_combo_->setCurrentIndex(1);
  }

  // Connect signals
  connect(model_a_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelComparisonDialog::OnModelAChanged);
  connect(model_b_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelComparisonDialog::OnModelBChanged);
  connect(image_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelComparisonDialog::OnImageSelected);
  connect(prev_button_, &QPushButton::clicked, this, &ModelComparisonDialog::OnPreviousImage);
  connect(next_button_, &QPushButton::clicked, this, &ModelComparisonDialog::OnNextImage);
  connect(compare_button_, &QPushButton::clicked, this, &ModelComparisonDialog::RunComparison);
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
  image_combo_->clear();
  for (const QString& img : test_images_)
  {
    image_combo_->addItem(img);
  }

  if (test_images_.isEmpty())
  {
    QMessageBox::warning(this, "No Test Images",
                         "No images are assigned to the test split.\n\n"
                         "Please configure dataset splits in Project Settings.");
    compare_button_->setEnabled(false);
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
    canvas_a_->setPixmap(pixmap);
    canvas_b_->setPixmap(pixmap);
    canvas_a_->ClearAllPolygons();
    canvas_b_->ClearAllPolygons();

    // Reset stats
    stats_a_->setText("Not yet run");
    stats_b_->setText("Not yet run");
  }

  // Update combo box
  image_combo_->setCurrentIndex(index);

  // Update navigation buttons
  prev_button_->setEnabled(index > 0);
  next_button_->setEnabled(index < test_images_.size() - 1);
}

void ModelComparisonDialog::RunDetectionOnModel(const QString& model_path, PolygonCanvas* canvas)
{
  // TODO: This should call the plugin with the specific model
  // For now, just show a placeholder
  Q_UNUSED(model_path);
  Q_UNUSED(canvas);

  QMessageBox::information(
      this, "Detection Placeholder",
      QString("Would run detection using model:\n%1\n\nThis requires plugin integration.").arg(model_path));
}

void ModelComparisonDialog::RunComparison()
{
  if (test_images_.isEmpty())
  {
    return;
  }

  const QList<ModelVersion>& models = config_.GetModelVersions();

  int model_a_idx = model_a_combo_->currentIndex();
  int model_b_idx = model_b_combo_->currentIndex();

  if (model_a_idx < 0 || model_a_idx >= models.size() || model_b_idx < 0 ||
      model_b_idx >= models.size())
  {
    QMessageBox::warning(this, "Invalid Selection", "Please select both models.");
    return;
  }

  QString model_a_path = project_dir_ + "/" + models[model_a_idx].path;
  QString model_b_path = project_dir_ + "/" + models[model_b_idx].path;

  // Clear previous results
  canvas_a_->ClearAllPolygons();
  canvas_b_->ClearAllPolygons();

  // Run detection on both models
  RunDetectionOnModel(model_a_path, canvas_a_);
  RunDetectionOnModel(model_b_path, canvas_b_);

  // Update stats (placeholder for now)
  int count_a = canvas_a_->GetPolygons().size();
  int count_b = canvas_b_->GetPolygons().size();

  stats_a_->setText(QString("Detections: %1").arg(count_a));
  stats_b_->setText(QString("Detections: %1").arg(count_b));
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
  canvas_a_->ClearAllPolygons();
  stats_a_->setText("Not yet run");
}

void ModelComparisonDialog::OnModelBChanged(int index)
{
  Q_UNUSED(index);
  // Reset results when model changes
  canvas_b_->ClearAllPolygons();
  stats_b_->setText("Not yet run");
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

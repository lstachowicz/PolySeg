#include "modelregistrationdialog.h"

#include <QDir>
#include <QFileDialog>

#include "ui_modelregistrationdialog.h"

ModelRegistrationDialog::ModelRegistrationDialog(const QString& project_dir, int default_model_count,
                                                 int labeled_count, QWidget* parent)
    : QDialog(parent), ui_(new Ui::ModelRegistrationDialog), project_dir_(project_dir)
{
  ui_->setupUi(this);

  // Set default values
  QString default_name = QString("model_v%1_%2imgs").arg(default_model_count).arg(labeled_count);
  ui_->name_edit_->setText(default_name);
  ui_->path_edit_->setText("models/best.pt");
  ui_->count_edit_->setText(QString::number(labeled_count));

  connect(ui_->browse_button_, &QPushButton::clicked, this, &ModelRegistrationDialog::OnBrowse);
}

ModelRegistrationDialog::~ModelRegistrationDialog()
{
  delete ui_;
}

QString ModelRegistrationDialog::GetModelName() const
{
  return ui_->name_edit_->text().trimmed();
}

QString ModelRegistrationDialog::GetModelPath() const
{
  QString path = ui_->path_edit_->text().trimmed();
  return path.isEmpty() ? "models/best.pt" : path;
}

QString ModelRegistrationDialog::GetNotes() const
{
  return ui_->notes_edit_->toPlainText().trimmed();
}

int ModelRegistrationDialog::GetTrainingCount() const
{
  return ui_->count_edit_->text().toInt();
}

void ModelRegistrationDialog::OnBrowse()
{
  QString models_dir = project_dir_ + "/models";
  QString file = QFileDialog::getOpenFileName(this, "Select Model File", models_dir,
                                              "Model Files (*.pt *.pth *.onnx *.h5)");
  if (!file.isEmpty())
  {
    // Make relative to project directory
    QDir project_dir(project_dir_);
    ui_->path_edit_->setText(project_dir.relativeFilePath(file));
  }
}

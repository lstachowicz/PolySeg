#ifndef MODELREGISTRATIONDIALOG_H
#define MODELREGISTRATIONDIALOG_H

#include <QDialog>

namespace Ui
{
class ModelRegistrationDialog;
}

/**
 * @brief Dialog for registering a trained model version
 *
 * Allows users to specify model name, path, and notes when
 * registering a newly trained model.
 */
class ModelRegistrationDialog : public QDialog
{
  Q_OBJECT

 public:
  explicit ModelRegistrationDialog(const QString& project_dir, int default_model_count,
                                   int labeled_count, QWidget* parent = nullptr);
  ~ModelRegistrationDialog();

  QString GetModelName() const;
  QString GetModelPath() const;
  QString GetNotes() const;
  int GetTrainingCount() const;

 private slots:
  void OnBrowse();

 private:
  Ui::ModelRegistrationDialog* ui_;
  QString project_dir_;
};

#endif  // MODELREGISTRATIONDIALOG_H

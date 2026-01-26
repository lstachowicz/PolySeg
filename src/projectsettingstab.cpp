#include "projectsettingstab.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>

#include "projectconfig.h"
#include "ui_projectsettingstab.h"

ProjectSettingsTab::ProjectSettingsTab(ProjectConfig& config, const QString& project_dir,
                                       QWidget* parent)
    : BaseSettingsTab(parent),
      ui_(new Ui::ProjectSettingsTab),
      config_(config),
      project_dir_(project_dir)
{
}

ProjectSettingsTab::~ProjectSettingsTab()
{
  delete ui_;
}

void ProjectSettingsTab::SetupUI()
{
  ui_->setupUi(this);

  // Configure table headers
  ui_->classes_table_->horizontalHeader()->setStretchLastSection(true);
}

void ProjectSettingsTab::ConnectSignals()
{
  // Class buttons
  connect(ui_->add_class_button_, &QPushButton::clicked, this, &ProjectSettingsTab::OnAddClass);
  connect(ui_->edit_class_button_, &QPushButton::clicked, this, &ProjectSettingsTab::OnEditClass);
  connect(ui_->remove_class_button_, &QPushButton::clicked, this, &ProjectSettingsTab::OnRemoveClass);
  connect(ui_->move_up_button_, &QPushButton::clicked, this, &ProjectSettingsTab::OnMoveClassUp);
  connect(ui_->move_down_button_, &QPushButton::clicked, this, &ProjectSettingsTab::OnMoveClassDown);

  // Browse buttons
  connect(ui_->browse_images_button_, &QPushButton::clicked, this,
          &ProjectSettingsTab::OnBrowseImagesFolder);
  connect(ui_->browse_labels_button_, &QPushButton::clicked, this,
          &ProjectSettingsTab::OnBrowseLabelsFolder);
}

void ProjectSettingsTab::LoadFromConfig(const ProjectConfig& config)
{
  // Basic settings
  ui_->project_name_edit_->setText(config.GetProjectName());

  // Annotation type
  AnnotationType anno_type = config.GetAnnotationType();
  ui_->annotation_type_combo_->setCurrentIndex(static_cast<int>(anno_type));

  // Classes
  const auto& classes = config.GetClasses();
  ui_->classes_table_->setRowCount(classes.size());
  for (int i = 0; i < classes.size(); ++i)
  {
    const ProjectClass& pc = classes[i];

    // Index column
    QTableWidgetItem* index_item = new QTableWidgetItem(QString::number(pc.index));
    index_item->setTextAlignment(Qt::AlignCenter);
    ui_->classes_table_->setItem(i, 0, index_item);

    // Name column
    ui_->classes_table_->setItem(i, 1, new QTableWidgetItem(pc.name));

    // Color column
    QTableWidgetItem* color_item = new QTableWidgetItem("");
    color_item->setBackground(QBrush(pc.color));
    color_item->setFlags(color_item->flags() & ~Qt::ItemIsEditable);
    ui_->classes_table_->setItem(i, 2, color_item);
  }
  ui_->classes_table_->resizeColumnToContents(0);

  // Custom folder paths (using defaults for now)
  ui_->images_folder_edit_->setText("images");
  ui_->labels_folder_edit_->setText("labels");
}

void ProjectSettingsTab::SaveToConfig(ProjectConfig& config)
{
  // Basic settings
  config.SetProjectName(ui_->project_name_edit_->text());

  // Annotation type
  int anno_idx = ui_->annotation_type_combo_->currentIndex();
  config.SetAnnotationType(static_cast<AnnotationType>(anno_idx));
}

void ProjectSettingsTab::OnAddClass()
{
  bool ok;
  QString name =
      QInputDialog::getText(this, "Add Class", "Class name:", QLineEdit::Normal, "", &ok);
  if (!ok || name.isEmpty())
    return;

  QColor color = QColorDialog::getColor(Qt::red, this, "Select Class Color");
  if (!color.isValid())
    return;

  config_.AddClass(name, color);
  LoadFromConfig(config_);  // Refresh UI
  emit classesChanged();
}

void ProjectSettingsTab::OnEditClass()
{
  int row = ui_->classes_table_->currentRow();
  if (row < 0)
  {
    QMessageBox::information(this, "Edit Class", "Please select a class first.");
    return;
  }

  const auto& classes = config_.GetClasses();
  if (row >= classes.size())
    return;

  int class_id = classes[row].id;
  ProjectClass* pc = config_.GetClass(class_id);
  if (!pc)
    return;

  bool ok;
  QString name =
      QInputDialog::getText(this, "Edit Class", "Class name:", QLineEdit::Normal, pc->name, &ok);
  if (!ok || name.isEmpty())
    return;

  QColor color = QColorDialog::getColor(pc->color, this, "Select Class Color");
  if (!color.isValid())
    return;

  config_.UpdateClass(class_id, name, color);
  LoadFromConfig(config_);  // Refresh UI
  ui_->classes_table_->selectRow(row);
  emit classesChanged();
}

void ProjectSettingsTab::OnRemoveClass()
{
  int row = ui_->classes_table_->currentRow();
  if (row < 0)
  {
    QMessageBox::information(this, "Remove Class", "Please select a class first.");
    return;
  }

  const auto& classes = config_.GetClasses();
  if (row >= classes.size())
    return;

  int class_id = classes[row].id;
  ProjectClass* pc = config_.GetClass(class_id);
  if (!pc)
    return;

  auto reply = QMessageBox::question(this, "Remove Class", "Remove class '" + pc->name + "'?",
                                      QMessageBox::Yes | QMessageBox::No);
  if (reply == QMessageBox::Yes)
  {
    config_.RemoveClass(class_id);
    config_.ReindexClasses();
    LoadFromConfig(config_);  // Refresh UI
    emit classesChanged();
  }
}

void ProjectSettingsTab::OnMoveClassUp()
{
  int row = ui_->classes_table_->currentRow();
  if (row <= 0)
    return;

  const auto& classes = config_.GetClasses();
  if (row >= classes.size())
    return;

  int class_id = classes[row].id;
  config_.MoveClass(class_id, -1);  // Move up
  LoadFromConfig(config_);          // Refresh UI
  ui_->classes_table_->selectRow(row - 1);
  emit classesChanged();
}

void ProjectSettingsTab::OnMoveClassDown()
{
  int row = ui_->classes_table_->currentRow();
  const auto& classes = config_.GetClasses();

  if (row < 0 || row >= classes.size() - 1)
    return;

  int class_id = classes[row].id;
  config_.MoveClass(class_id, 1);  // Move down
  LoadFromConfig(config_);         // Refresh UI
  ui_->classes_table_->selectRow(row + 1);
  emit classesChanged();
}

void ProjectSettingsTab::OnBrowseImagesFolder()
{
  QString dir = QFileDialog::getExistingDirectory(this, "Select Images Folder");
  if (!dir.isEmpty())
    ui_->images_folder_edit_->setText(dir);
}

void ProjectSettingsTab::OnBrowseLabelsFolder()
{
  QString dir = QFileDialog::getExistingDirectory(this, "Select Labels Folder");
  if (!dir.isEmpty())
    ui_->labels_folder_edit_->setText(dir);
}

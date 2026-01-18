#include "projectsettingstab.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>

#include "projectconfig.h"

ProjectSettingsTab::ProjectSettingsTab(ProjectConfig& config, const QString& project_dir,
                                       QWidget* parent)
    : BaseSettingsTab(parent),
      config_(config),
      project_dir_(project_dir),
      project_name_edit_(nullptr),
      annotation_type_combo_(nullptr),
      auto_save_checkbox_(nullptr),
      auto_save_interval_spinbox_(nullptr),
      classes_table_(nullptr),
      add_class_button_(nullptr),
      edit_class_button_(nullptr),
      remove_class_button_(nullptr),
      move_up_button_(nullptr),
      move_down_button_(nullptr),
      images_folder_edit_(nullptr),
      labels_folder_edit_(nullptr),
      browse_images_button_(nullptr),
      browse_labels_button_(nullptr)
{
}

void ProjectSettingsTab::SetupUI()
{
  QVBoxLayout* main_layout = GetMainLayout();

  // ===== Basic Settings =====
  QGroupBox* basic_group = new QGroupBox("Basic Settings");
  QFormLayout* basic_layout = new QFormLayout(basic_group);

  project_name_edit_ = new QLineEdit();
  basic_layout->addRow("Project Name:", project_name_edit_);

  annotation_type_combo_ = new QComboBox();
  annotation_type_combo_->addItem("Polygon", static_cast<int>(AnnotationType::Polygon));
  annotation_type_combo_->addItem("Bounding Box", static_cast<int>(AnnotationType::BoundingBox));
  basic_layout->addRow("Annotation Type:", annotation_type_combo_);

  main_layout->addWidget(basic_group);

  // ===== Auto-Save Settings =====
  QGroupBox* auto_save_group = new QGroupBox("Auto-Save Settings");
  QVBoxLayout* auto_save_layout = new QVBoxLayout(auto_save_group);

  auto_save_checkbox_ = new QCheckBox("Enable auto-save");
  auto_save_checkbox_->setChecked(true);
  auto_save_layout->addWidget(auto_save_checkbox_);

  QHBoxLayout* interval_layout = new QHBoxLayout();
  QLabel* interval_label = new QLabel("Auto-save interval:");
  auto_save_interval_spinbox_ = new QSpinBox();
  auto_save_interval_spinbox_->setRange(10, 300);
  auto_save_interval_spinbox_->setSuffix(" seconds");
  auto_save_interval_spinbox_->setValue(30);

  interval_layout->addWidget(interval_label);
  interval_layout->addWidget(auto_save_interval_spinbox_);
  interval_layout->addStretch();

  auto_save_layout->addLayout(interval_layout);
  main_layout->addWidget(auto_save_group);

  // ===== Classes =====
  QGroupBox* classes_group = new QGroupBox("Annotation Classes");
  QVBoxLayout* classes_layout = new QVBoxLayout(classes_group);

  QLabel* classes_info =
      new QLabel("Define annotation classes. Use keyboard shortcuts 1-9 to quickly select classes.");
  classes_info->setWordWrap(true);
  classes_info->setStyleSheet("color: gray; font-size: 10px;");
  classes_layout->addWidget(classes_info);

  classes_table_ = new QTableWidget();
  classes_table_->setColumnCount(3);
  classes_table_->setHorizontalHeaderLabels({"Index", "Name", "Color"});
  classes_table_->horizontalHeader()->setStretchLastSection(true);
  classes_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  classes_table_->setMaximumHeight(200);
  classes_layout->addWidget(classes_table_);

  QHBoxLayout* class_btn_layout = new QHBoxLayout();

  move_up_button_ = new QPushButton("↑");
  move_up_button_->setMaximumWidth(40);
  move_down_button_ = new QPushButton("↓");
  move_down_button_->setMaximumWidth(40);

  class_btn_layout->addWidget(move_up_button_);
  class_btn_layout->addWidget(move_down_button_);
  class_btn_layout->addStretch();

  add_class_button_ = new QPushButton("Add Class");
  edit_class_button_ = new QPushButton("Edit Class");
  remove_class_button_ = new QPushButton("Remove Class");

  class_btn_layout->addWidget(add_class_button_);
  class_btn_layout->addWidget(edit_class_button_);
  class_btn_layout->addWidget(remove_class_button_);

  classes_layout->addLayout(class_btn_layout);
  main_layout->addWidget(classes_group);

  // ===== Custom Folder Paths =====
  QGroupBox* paths_group = new QGroupBox("Custom Folder Paths");
  QFormLayout* paths_layout = new QFormLayout(paths_group);

  QHBoxLayout* images_layout = new QHBoxLayout();
  images_folder_edit_ = new QLineEdit();
  images_folder_edit_->setPlaceholderText("images");
  browse_images_button_ = new QPushButton("Browse...");
  images_layout->addWidget(images_folder_edit_, 1);
  images_layout->addWidget(browse_images_button_);
  paths_layout->addRow("Images Folder:", images_layout);

  QHBoxLayout* labels_layout = new QHBoxLayout();
  labels_folder_edit_ = new QLineEdit();
  labels_folder_edit_->setPlaceholderText("labels");
  browse_labels_button_ = new QPushButton("Browse...");
  labels_layout->addWidget(labels_folder_edit_, 1);
  labels_layout->addWidget(browse_labels_button_);
  paths_layout->addRow("Labels Folder:", labels_layout);

  main_layout->addWidget(paths_group);

  main_layout->addStretch();
}

void ProjectSettingsTab::ConnectSignals()
{
  // Class buttons
  connect(add_class_button_, &QPushButton::clicked, this, &ProjectSettingsTab::OnAddClass);
  connect(edit_class_button_, &QPushButton::clicked, this, &ProjectSettingsTab::OnEditClass);
  connect(remove_class_button_, &QPushButton::clicked, this, &ProjectSettingsTab::OnRemoveClass);
  connect(move_up_button_, &QPushButton::clicked, this, &ProjectSettingsTab::OnMoveClassUp);
  connect(move_down_button_, &QPushButton::clicked, this, &ProjectSettingsTab::OnMoveClassDown);

  // Browse buttons
  connect(browse_images_button_, &QPushButton::clicked, this,
          &ProjectSettingsTab::OnBrowseImagesFolder);
  connect(browse_labels_button_, &QPushButton::clicked, this,
          &ProjectSettingsTab::OnBrowseLabelsFolder);
}

void ProjectSettingsTab::LoadFromConfig(const ProjectConfig& config)
{
  // Basic settings
  project_name_edit_->setText(config.GetProjectName());

  // Annotation type
  AnnotationType anno_type = config.GetAnnotationType();
  annotation_type_combo_->setCurrentIndex(static_cast<int>(anno_type));

  // Classes
  const auto& classes = config.GetClasses();
  classes_table_->setRowCount(classes.size());
  for (int i = 0; i < classes.size(); ++i)
  {
    const ProjectClass& pc = classes[i];

    // Index column
    QTableWidgetItem* index_item = new QTableWidgetItem(QString::number(pc.index));
    index_item->setTextAlignment(Qt::AlignCenter);
    classes_table_->setItem(i, 0, index_item);

    // Name column
    classes_table_->setItem(i, 1, new QTableWidgetItem(pc.name));

    // Color column
    QTableWidgetItem* color_item = new QTableWidgetItem("");
    color_item->setBackground(QBrush(pc.color));
    color_item->setFlags(color_item->flags() & ~Qt::ItemIsEditable);
    classes_table_->setItem(i, 2, color_item);
  }
  classes_table_->resizeColumnToContents(0);

  // Custom folder paths (using defaults for now)
  images_folder_edit_->setText("images");
  labels_folder_edit_->setText("labels");
}

void ProjectSettingsTab::SaveToConfig(ProjectConfig& config)
{
  // Basic settings
  config.SetProjectName(project_name_edit_->text());

  // Annotation type
  int anno_idx = annotation_type_combo_->currentIndex();
  config.SetAnnotationType(static_cast<AnnotationType>(anno_idx));

  // Note: Custom folder paths would be saved here if we had getters for them
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
  int row = classes_table_->currentRow();
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
  classes_table_->selectRow(row);
  emit classesChanged();
}

void ProjectSettingsTab::OnRemoveClass()
{
  int row = classes_table_->currentRow();
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
  int row = classes_table_->currentRow();
  if (row <= 0)
    return;

  const auto& classes = config_.GetClasses();
  if (row >= classes.size())
    return;

  int class_id = classes[row].id;
  config_.MoveClass(class_id, -1);  // Move up
  LoadFromConfig(config_);          // Refresh UI
  classes_table_->selectRow(row - 1);
  emit classesChanged();
}

void ProjectSettingsTab::OnMoveClassDown()
{
  int row = classes_table_->currentRow();
  const auto& classes = config_.GetClasses();

  if (row < 0 || row >= classes.size() - 1)
    return;

  int class_id = classes[row].id;
  config_.MoveClass(class_id, 1);  // Move down
  LoadFromConfig(config_);         // Refresh UI
  classes_table_->selectRow(row + 1);
  emit classesChanged();
}

void ProjectSettingsTab::OnBrowseImagesFolder()
{
  QString dir = QFileDialog::getExistingDirectory(this, "Select Images Folder");
  if (!dir.isEmpty())
    images_folder_edit_->setText(dir);
}

void ProjectSettingsTab::OnBrowseLabelsFolder()
{
  QString dir = QFileDialog::getExistingDirectory(this, "Select Labels Folder");
  if (!dir.isEmpty())
    labels_folder_edit_->setText(dir);
}

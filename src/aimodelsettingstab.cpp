#include "aimodelsettingstab.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "modelcomparisondialog.h"
#include "projectconfig.h"

AIModelSettingsTab::AIModelSettingsTab(ProjectConfig& config, const QString& project_dir,
                                       QWidget* parent)
    : BaseSettingsTab(parent), config_(config), project_dir_(project_dir)
{
}

void AIModelSettingsTab::SetupUI()
{
  QVBoxLayout* main_layout = GetMainLayout();

  // ===== Plugin Configuration =====
  QGroupBox* plugin_group = new QGroupBox("AI Plugin Configuration");
  QVBoxLayout* plugin_layout = new QVBoxLayout(plugin_group);

  QLabel* plugin_info = new QLabel(
      "Configure AI plugin for automatic detection and model training. "
      "Supports any framework (PyTorch, TensorFlow, ONNX) via command-line interface.");
  plugin_info->setWordWrap(true);
  plugin_info->setStyleSheet("color: gray; font-size: 10px;");
  plugin_layout->addWidget(plugin_info);

  plugin_enabled_checkbox_ = new QCheckBox("Enable AI Plugin");
  plugin_layout->addWidget(plugin_enabled_checkbox_);

  QFormLayout* plugin_form = new QFormLayout();

  plugin_name_edit_ = new QLineEdit();
  plugin_form->addRow("Plugin Name:", plugin_name_edit_);

  plugin_env_setup_edit_ = new QLineEdit();
  plugin_env_setup_edit_->setPlaceholderText("e.g., source venv/bin/activate");
  plugin_form->addRow("Env Setup:", plugin_env_setup_edit_);

  plugin_command_edit_ = new QLineEdit();
  plugin_form->addRow("Command:", plugin_command_edit_);

  QHBoxLayout* script_layout = new QHBoxLayout();
  plugin_script_edit_ = new QLineEdit();
  browse_script_button_ = new QPushButton("Browse...");
  script_layout->addWidget(plugin_script_edit_);
  script_layout->addWidget(browse_script_button_);
  plugin_form->addRow("Script Path:", script_layout);

  plugin_detect_args_edit_ = new QLineEdit();
  plugin_form->addRow("Detect Args:", plugin_detect_args_edit_);

  plugin_train_args_edit_ = new QLineEdit();
  plugin_form->addRow("Train Args:", plugin_train_args_edit_);

  plugin_layout->addLayout(plugin_form);

  // Plugin Settings
  QGroupBox* settings_subgroup = new QGroupBox("Plugin Settings");
  QVBoxLayout* settings_subgroup_layout = new QVBoxLayout(settings_subgroup);

  QLabel* settings_info = new QLabel(
      "Add custom key-value settings for your plugin (e.g., model path, confidence threshold). "
      "These will be available as {key} variables in Detect/Train Args.");
  settings_info->setWordWrap(true);
  settings_info->setStyleSheet("color: gray; font-size: 10px;");
  settings_subgroup_layout->addWidget(settings_info);

  plugin_settings_layout_ = new QFormLayout();
  add_plugin_setting_button_ = new QPushButton("Add Setting");
  plugin_settings_layout_->addRow("", add_plugin_setting_button_);
  settings_subgroup_layout->addLayout(plugin_settings_layout_);

  plugin_layout->addWidget(settings_subgroup);

  QLabel* help_label =
      new QLabel("<b>Variable Substitution:</b> {image}, {project}, {model}, {confidence}, or any custom {key}");
  help_label->setWordWrap(true);
  help_label->setStyleSheet("color: gray; font-size: 10px;");
  plugin_layout->addWidget(help_label);

  main_layout->addWidget(plugin_group);

  // ===== Dataset Splits =====
  QGroupBox* splits_group = new QGroupBox("Dataset Splits (Train/Val/Test)");
  QVBoxLayout* splits_layout = new QVBoxLayout(splits_group);

  QLabel* splits_info = new QLabel(
      "Configure train/validation/test splits for model training. "
      "Splits are deterministic based on filename hash.");
  splits_info->setWordWrap(true);
  splits_info->setStyleSheet("color: gray; font-size: 10px;");
  splits_layout->addWidget(splits_info);

  splits_enabled_checkbox_ = new QCheckBox("Enable train/val/test splits");
  splits_layout->addWidget(splits_enabled_checkbox_);

  QFormLayout* ratios_layout = new QFormLayout();

  QHBoxLayout* train_layout = new QHBoxLayout();
  train_ratio_slider_ = new QSlider(Qt::Horizontal);
  train_ratio_slider_->setRange(0, 100);
  train_ratio_slider_->setValue(70);
  train_ratio_label_ = new QLabel("70%");
  train_layout->addWidget(train_ratio_slider_, 1);
  train_layout->addWidget(train_ratio_label_);
  ratios_layout->addRow("Train:", train_layout);

  QHBoxLayout* val_layout = new QHBoxLayout();
  val_ratio_slider_ = new QSlider(Qt::Horizontal);
  val_ratio_slider_->setRange(0, 100);
  val_ratio_slider_->setValue(20);
  val_ratio_label_ = new QLabel("20%");
  val_layout->addWidget(val_ratio_slider_, 1);
  val_layout->addWidget(val_ratio_label_);
  ratios_layout->addRow("Validation:", val_layout);

  QHBoxLayout* test_layout = new QHBoxLayout();
  test_ratio_slider_ = new QSlider(Qt::Horizontal);
  test_ratio_slider_->setRange(0, 100);
  test_ratio_slider_->setValue(10);
  test_ratio_label_ = new QLabel("10%");
  test_layout->addWidget(test_ratio_slider_, 1);
  test_layout->addWidget(test_ratio_label_);
  ratios_layout->addRow("Test:", test_layout);

  splits_layout->addLayout(ratios_layout);

  split_statistics_label_ = new QLabel("Target: 70/20/10% | Actual: calculating...");
  split_statistics_label_->setStyleSheet(
      "QLabel { padding: 10px; background-color: #f0f0f0; border-radius: 5px; }");
  splits_layout->addWidget(split_statistics_label_);

  QHBoxLayout* salt_layout = new QHBoxLayout();
  salt_layout->addWidget(new QLabel("Random Seed (Salt):"));
  salt_edit_ = new QLineEdit();
  salt_edit_->setReadOnly(true);
  salt_layout->addWidget(salt_edit_, 1);
  splits_layout->addLayout(salt_layout);

  QHBoxLayout* reset_layout = new QHBoxLayout();
  reset_layout->addStretch();
  reset_splits_button_ = new QPushButton("Reset All Splits");
  reset_splits_button_->setStyleSheet("QPushButton { color: red; }");
  reset_layout->addWidget(reset_splits_button_);
  splits_layout->addLayout(reset_layout);

  main_layout->addWidget(splits_group);

  // ===== Model Versions =====
  QGroupBox* models_group = new QGroupBox("Model Versions");
  QVBoxLayout* models_layout = new QVBoxLayout(models_group);

  QLabel* models_info = new QLabel(
      "Track trained model versions with metadata. Register models after training to compare "
      "performance and maintain version history.");
  models_info->setWordWrap(true);
  models_info->setStyleSheet("color: gray; font-size: 10px;");
  models_layout->addWidget(models_info);

  model_versions_table_ = new QTableWidget();
  model_versions_table_->setColumnCount(5);
  model_versions_table_->setHorizontalHeaderLabels(
      {"Name", "Date", "Images Count", "Path", "Notes"});
  model_versions_table_->horizontalHeader()->setStretchLastSection(true);
  model_versions_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  model_versions_table_->setSelectionMode(QAbstractItemView::SingleSelection);
  model_versions_table_->setMaximumHeight(250);
  models_layout->addWidget(model_versions_table_);

  QHBoxLayout* model_btn_layout = new QHBoxLayout();
  add_model_button_ = new QPushButton("Add Model");
  edit_notes_button_ = new QPushButton("Edit Notes");
  compare_models_button_ = new QPushButton("Compare Models...");
  remove_model_button_ = new QPushButton("Remove Entry");

  model_btn_layout->addWidget(add_model_button_);
  model_btn_layout->addWidget(edit_notes_button_);
  model_btn_layout->addWidget(compare_models_button_);
  model_btn_layout->addStretch();
  model_btn_layout->addWidget(remove_model_button_);

  models_layout->addLayout(model_btn_layout);
  main_layout->addWidget(models_group);

  main_layout->addStretch();
}

void AIModelSettingsTab::ConnectSignals()
{
  // Plugin signals
  connect(browse_script_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnBrowsePluginScript);
  connect(add_plugin_setting_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnAddPluginSetting);

  // Split signals
  connect(splits_enabled_checkbox_, &QCheckBox::stateChanged, this,
          &AIModelSettingsTab::OnSplitsEnabledChanged);
  connect(train_ratio_slider_, &QSlider::valueChanged, this,
          &AIModelSettingsTab::OnSplitRatioChanged);
  connect(val_ratio_slider_, &QSlider::valueChanged, this, &AIModelSettingsTab::OnSplitRatioChanged);
  connect(test_ratio_slider_, &QSlider::valueChanged, this,
          &AIModelSettingsTab::OnSplitRatioChanged);
  connect(reset_splits_button_, &QPushButton::clicked, this, &AIModelSettingsTab::OnResetSplits);

  // Model signals
  connect(add_model_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnAddModelVersion);
  connect(edit_notes_button_, &QPushButton::clicked, this, &AIModelSettingsTab::OnEditModelNotes);
  connect(compare_models_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnCompareModels);
  connect(remove_model_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnRemoveModelVersion);
}

void AIModelSettingsTab::LoadFromConfig(const ProjectConfig& config)
{
  // Plugin Configuration
  const PluginConfig& plugin = config.GetPluginConfig();
  plugin_enabled_checkbox_->setChecked(plugin.enabled);
  plugin_name_edit_->setText(plugin.name);
  plugin_env_setup_edit_->setText(plugin.env_setup);
  plugin_command_edit_->setText(plugin.command);
  plugin_script_edit_->setText(plugin.script_path);
  plugin_detect_args_edit_->setText(plugin.detect_args);
  plugin_train_args_edit_->setText(plugin.train_args);

  // Clear existing plugin settings
  plugin_setting_edits_.clear();
  // Remove all rows except the "Add Setting" button row
  while (plugin_settings_layout_->rowCount() > 1)
  {
    plugin_settings_layout_->removeRow(0);
  }

  // Add plugin settings from config
  for (auto it = plugin.settings.begin(); it != plugin.settings.end(); ++it)
  {
    QLineEdit* edit = new QLineEdit(it.value());
    plugin_settings_layout_->insertRow(plugin_settings_layout_->rowCount() - 1, it.key() + ":",
                                       edit);
    plugin_setting_edits_[it.key()] = edit;
  }

  // Dataset Splits
  const SplitConfig& split_cfg = config.GetSplitConfig();
  splits_enabled_checkbox_->setChecked(split_cfg.enabled);
  train_ratio_slider_->setValue(static_cast<int>(split_cfg.target_train_ratio * 100));
  val_ratio_slider_->setValue(static_cast<int>(split_cfg.target_val_ratio * 100));
  test_ratio_slider_->setValue(static_cast<int>(split_cfg.target_test_ratio * 100));
  salt_edit_->setText(split_cfg.hash_salt);
  OnSplitRatioChanged();  // Update labels
  UpdateSplitStatistics();

  // Model Versions
  RefreshModelList();
}

void AIModelSettingsTab::SaveToConfig(ProjectConfig& config)
{
  // Plugin Configuration
  PluginConfig plugin = config.GetPluginConfig();
  plugin.enabled = plugin_enabled_checkbox_->isChecked();
  plugin.name = plugin_name_edit_->text();
  plugin.env_setup = plugin_env_setup_edit_->text();
  plugin.command = plugin_command_edit_->text();
  plugin.script_path = plugin_script_edit_->text();
  plugin.detect_args = plugin_detect_args_edit_->text();
  plugin.train_args = plugin_train_args_edit_->text();

  // Save plugin settings
  plugin.settings.clear();
  for (auto it = plugin_setting_edits_.begin(); it != plugin_setting_edits_.end(); ++it)
  {
    plugin.settings[it.key()] = it.value()->text();
  }

  // Save plugin config
  config.SetPluginConfig(plugin);

  // Dataset Splits
  SplitConfig split_cfg;
  split_cfg.enabled = splits_enabled_checkbox_->isChecked();
  split_cfg.target_train_ratio = train_ratio_slider_->value() / 100.0;
  split_cfg.target_val_ratio = val_ratio_slider_->value() / 100.0;
  split_cfg.target_test_ratio = test_ratio_slider_->value() / 100.0;
  split_cfg.hash_salt = salt_edit_->text();
  config.SetSplitConfig(split_cfg);
}

void AIModelSettingsTab::RefreshModelList()
{
  // Reload model list from current config
  const QList<ModelVersion>& models = config_.GetModelVersions();
  model_versions_table_->setRowCount(models.size());
  for (int i = 0; i < models.size(); ++i)
  {
    const ModelVersion& model = models[i];
    model_versions_table_->setItem(i, 0, new QTableWidgetItem(model.name));
    model_versions_table_->setItem(i, 1,
                                    new QTableWidgetItem(model.timestamp.toString("yyyy-MM-dd")));
    model_versions_table_->setItem(i, 2,
                                    new QTableWidgetItem(QString::number(model.training_images_count)));
    model_versions_table_->setItem(i, 3, new QTableWidgetItem(model.path));
    model_versions_table_->setItem(i, 4, new QTableWidgetItem(model.notes));
  }
}

void AIModelSettingsTab::OnBrowsePluginScript()
{
  QString file = QFileDialog::getOpenFileName(this, "Select Plugin Script", project_dir_,
                                              "Python Scripts (*.py);;All Files (*)");
  if (!file.isEmpty())
  {
    // Make path relative to project if possible
    if (file.startsWith(project_dir_))
    {
      file = file.mid(project_dir_.length() + 1);
    }
    plugin_script_edit_->setText(file);
  }
}

void AIModelSettingsTab::OnAddPluginSetting()
{
  bool ok;
  QString key =
      QInputDialog::getText(this, "Add Setting", "Setting name:", QLineEdit::Normal, "", &ok);
  if (ok && !key.isEmpty())
  {
    if (plugin_setting_edits_.contains(key))
    {
      QMessageBox::warning(this, "Duplicate Setting",
                           QString("Setting '%1' already exists!").arg(key));
      return;
    }

    QLineEdit* edit = new QLineEdit();
    // Insert before the "Add Setting" button row
    plugin_settings_layout_->insertRow(plugin_settings_layout_->rowCount() - 1, key + ":", edit);
    plugin_setting_edits_[key] = edit;
  }
}

void AIModelSettingsTab::OnAddModelVersion()
{
  // Emit signal to parent window to handle registration
  emit requestModelRegistration();

  // Note: RefreshModelList() will be called after registration completes
}

void AIModelSettingsTab::OnEditModelNotes()
{
  int row = model_versions_table_->currentRow();
  if (row < 0)
  {
    QMessageBox::warning(this, "No Selection", "Please select a model version to edit.");
    return;
  }

  const QList<ModelVersion>& models = config_.GetModelVersions();
  if (row >= models.size())
  {
    return;
  }

  ModelVersion model = models[row];

  // Create edit dialog
  QDialog dialog(this);
  dialog.setWindowTitle("Edit Model Notes - " + model.name);
  dialog.setMinimumSize(400, 250);

  QVBoxLayout* layout = new QVBoxLayout(&dialog);
  layout->addWidget(new QLabel("Notes:"));

  QTextEdit* notes_edit = new QTextEdit();
  notes_edit->setPlainText(model.notes);
  layout->addWidget(notes_edit);

  QHBoxLayout* button_layout = new QHBoxLayout();
  button_layout->addStretch();
  QPushButton* ok_button = new QPushButton("Save");
  QPushButton* cancel_button = new QPushButton("Cancel");
  button_layout->addWidget(ok_button);
  button_layout->addWidget(cancel_button);
  layout->addLayout(button_layout);

  connect(ok_button, &QPushButton::clicked, &dialog, &QDialog::accept);
  connect(cancel_button, &QPushButton::clicked, &dialog, &QDialog::reject);

  if (dialog.exec() == QDialog::Accepted)
  {
    model.notes = notes_edit->toPlainText().trimmed();
    config_.UpdateModelVersion(row, model);

    // Update table
    model_versions_table_->setItem(row, 4, new QTableWidgetItem(model.notes));

    QMessageBox::information(this, "Notes Updated", "Model notes have been updated.");
  }
}

void AIModelSettingsTab::OnRemoveModelVersion()
{
  int row = model_versions_table_->currentRow();
  if (row < 0)
  {
    QMessageBox::warning(this, "No Selection", "Please select a model version to remove.");
    return;
  }

  QMessageBox::StandardButton reply = QMessageBox::question(
      this, "Remove Model",
      "Remove this model version entry?\n(The model file will NOT be deleted from disk)",
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes)
  {
    config_.RemoveModelVersion(row);
    RefreshModelList();
  }
}

void AIModelSettingsTab::OnCompareModels()
{
  const QList<ModelVersion>& models = config_.GetModelVersions();

  if (models.size() < 2)
  {
    QMessageBox::information(this, "Not Enough Models",
                             "You need at least 2 registered models to compare.\n\n"
                             "Register models after training using the 'Add Model' button.");
    return;
  }

  // Open comparison dialog
  ModelComparisonDialog* dialog = new ModelComparisonDialog(config_, project_dir_, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
}

void AIModelSettingsTab::OnSplitsEnabledChanged(int state)
{
  bool enabled = (state == Qt::Checked);
  train_ratio_slider_->setEnabled(enabled);
  val_ratio_slider_->setEnabled(enabled);
  test_ratio_slider_->setEnabled(enabled);
  reset_splits_button_->setEnabled(enabled);

  SplitConfig split_cfg = config_.GetSplitConfig();
  split_cfg.enabled = enabled;
  config_.SetSplitConfig(split_cfg);
  UpdateSplitStatistics();
  emit splitsChanged();
}

void AIModelSettingsTab::OnSplitRatioChanged()
{
  // Update labels
  train_ratio_label_->setText(QString("%1%").arg(train_ratio_slider_->value()));
  val_ratio_label_->setText(QString("%1%").arg(val_ratio_slider_->value()));
  test_ratio_label_->setText(QString("%1%").arg(test_ratio_slider_->value()));

  // Update config
  SplitConfig split_cfg = config_.GetSplitConfig();
  split_cfg.target_train_ratio = train_ratio_slider_->value() / 100.0;
  split_cfg.target_val_ratio = val_ratio_slider_->value() / 100.0;
  split_cfg.target_test_ratio = test_ratio_slider_->value() / 100.0;
  config_.SetSplitConfig(split_cfg);

  UpdateSplitStatistics();
  emit splitsChanged();
}

void AIModelSettingsTab::OnResetSplits()
{
  QMessageBox::StandardButton reply = QMessageBox::warning(
      this, "Reset All Splits",
      "WARNING: This will clear all train/val/test assignments.\n"
      "Your test set will change completely!\n\n"
      "Existing models will be moved to models_old_TIMESTAMP/\n\n"
      "Are you sure you want to continue?",
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

  if (reply == QMessageBox::Yes)
  {
    config_.ResetAllSplits();
    UpdateSplitStatistics();
    QMessageBox::information(this, "Splits Reset", "All splits have been reset. Old models archived.");
    emit splitsChanged();
  }
}

void AIModelSettingsTab::UpdateSplitStatistics()
{
  const SplitConfig& split_cfg = config_.GetSplitConfig();
  if (!split_cfg.enabled)
  {
    split_statistics_label_->setText("Splits disabled");
    return;
  }

  // Calculate actual split counts
  int total_images = config_.GetImageFiles().size();
  QMap<QString, int> counts;
  counts["train"] = 0;
  counts["val"] = 0;
  counts["test"] = 0;

  for (const auto& img : config_.GetImageFiles())
  {
    QString split = config_.GetImageSplit(img);
    if (counts.contains(split))
    {
      counts[split]++;
    }
  }

  int target_train = static_cast<int>(train_ratio_slider_->value());
  int target_val = static_cast<int>(val_ratio_slider_->value());
  int target_test = static_cast<int>(test_ratio_slider_->value());

  int actual_train_pct = total_images > 0 ? (counts["train"] * 100 / total_images) : 0;
  int actual_val_pct = total_images > 0 ? (counts["val"] * 100 / total_images) : 0;
  int actual_test_pct = total_images > 0 ? (counts["test"] * 100 / total_images) : 0;

  QString stats = QString("Target: %1/%2/%3% | Actual: %4/%5/%6% (T:%7 V:%8 Te:%9)")
                      .arg(target_train)
                      .arg(target_val)
                      .arg(target_test)
                      .arg(actual_train_pct)
                      .arg(actual_val_pct)
                      .arg(actual_test_pct)
                      .arg(counts["train"])
                      .arg(counts["val"])
                      .arg(counts["test"]);

  split_statistics_label_->setText(stats);
}

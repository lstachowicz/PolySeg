#include "aimodelsettingstab.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "modelcomparisondialog.h"
#include "projectconfig.h"
#include "ui_aimodelsettingstab.h"

AIModelSettingsTab::AIModelSettingsTab(ProjectConfig& config, const QString& project_dir,
                                       QWidget* parent)
    : BaseSettingsTab(parent), ui_(new Ui::AIModelSettingsTab), config_(config), project_dir_(project_dir)
{
}

AIModelSettingsTab::~AIModelSettingsTab()
{
  delete ui_;
}

void AIModelSettingsTab::SetupUI()
{
  // Setup UI from .ui file
  ui_->setupUi(this);

  // Configure table headers to stretch
  ui_->plugin_settings_table_->horizontalHeader()->setStretchLastSection(true);
  ui_->model_versions_table_->horizontalHeader()->setStretchLastSection(true);
}

void AIModelSettingsTab::ConnectSignals()
{
  // Plugin signals
  connect(ui_->plugin_wizard_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::requestPluginWizard);
  connect(ui_->browse_script_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnBrowsePluginScript);
  connect(ui_->add_plugin_setting_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnAddPluginSetting);
  connect(ui_->remove_plugin_setting_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnRemovePluginSetting);

  // Split signals
  connect(ui_->splits_enabled_checkbox_, &QCheckBox::checkStateChanged, this,
          &AIModelSettingsTab::OnSplitsEnabledChanged);
  connect(ui_->train_ratio_slider_, &QSlider::valueChanged, this,
          &AIModelSettingsTab::OnSplitRatioChanged);
  connect(ui_->val_ratio_slider_, &QSlider::valueChanged, this, &AIModelSettingsTab::OnSplitRatioChanged);
  connect(ui_->test_ratio_slider_, &QSlider::valueChanged, this,
          &AIModelSettingsTab::OnSplitRatioChanged);
  connect(ui_->reset_splits_button_, &QPushButton::clicked, this, &AIModelSettingsTab::OnResetSplits);

  // Model signals
  connect(ui_->add_model_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnAddModelVersion);
  connect(ui_->edit_notes_button_, &QPushButton::clicked, this, &AIModelSettingsTab::OnEditModelNotes);
  connect(ui_->compare_models_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnCompareModels);
  connect(ui_->remove_model_button_, &QPushButton::clicked, this,
          &AIModelSettingsTab::OnRemoveModelVersion);
}

void AIModelSettingsTab::LoadFromConfig(const ProjectConfig& config)
{
  // Plugin Configuration
  const PluginConfig& plugin = config.GetPluginConfig();
  ui_->plugin_enabled_checkbox_->setChecked(plugin.enabled);
  ui_->plugin_name_edit_->setText(plugin.name);
  ui_->plugin_env_setup_edit_->setText(plugin.env_setup);
  ui_->plugin_command_edit_->setText(plugin.command);
  ui_->plugin_script_edit_->setText(plugin.script_path);
  ui_->plugin_detect_args_edit_->setText(plugin.detect_args);
  ui_->plugin_train_args_edit_->setText(plugin.train_args);

  // Populate plugin settings table
  PopulatePluginSettingsTable();

  // Dataset Splits
  const SplitConfig& split_cfg = config.GetSplitConfig();
  ui_->splits_enabled_checkbox_->setChecked(split_cfg.enabled);
  ui_->train_ratio_slider_->setValue(static_cast<int>(split_cfg.target_train_ratio * 100));
  ui_->val_ratio_slider_->setValue(static_cast<int>(split_cfg.target_val_ratio * 100));
  ui_->test_ratio_slider_->setValue(static_cast<int>(split_cfg.target_test_ratio * 100));
  ui_->salt_edit_->setText(split_cfg.hash_salt);
  OnSplitRatioChanged();  // Update labels
  UpdateSplitStatistics();

  // Model Versions
  RefreshModelList();
}

void AIModelSettingsTab::PopulatePluginSettingsTable()
{
  const PluginConfig& plugin = config_.GetPluginConfig();
  ui_->plugin_settings_table_->setRowCount(0);

  for (auto it = plugin.settings.begin(); it != plugin.settings.end(); ++it)
  {
    int row = ui_->plugin_settings_table_->rowCount();
    ui_->plugin_settings_table_->insertRow(row);
    ui_->plugin_settings_table_->setItem(row, 0, new QTableWidgetItem(it.key()));
    ui_->plugin_settings_table_->setItem(row, 1, new QTableWidgetItem(it.value()));
  }
}

QMap<QString, QString> AIModelSettingsTab::GetPluginSettingsFromTable() const
{
  QMap<QString, QString> settings;

  for (int row = 0; row < ui_->plugin_settings_table_->rowCount(); ++row)
  {
    QTableWidgetItem* key_item = ui_->plugin_settings_table_->item(row, 0);
    QTableWidgetItem* value_item = ui_->plugin_settings_table_->item(row, 1);

    if (key_item && value_item)
    {
      QString key = key_item->text().trimmed();
      QString value = value_item->text().trimmed();

      if (!key.isEmpty())
      {
        settings[key] = value;
      }
    }
  }

  return settings;
}

void AIModelSettingsTab::SaveToConfig(ProjectConfig& config)
{
  // Plugin Configuration
  PluginConfig plugin = config.GetPluginConfig();
  plugin.enabled = ui_->plugin_enabled_checkbox_->isChecked();
  plugin.name = ui_->plugin_name_edit_->text();
  plugin.env_setup = ui_->plugin_env_setup_edit_->text();
  plugin.command = ui_->plugin_command_edit_->text();
  plugin.script_path = ui_->plugin_script_edit_->text();
  plugin.detect_args = ui_->plugin_detect_args_edit_->text();
  plugin.train_args = ui_->plugin_train_args_edit_->text();

  // Save plugin settings from table
  plugin.settings = GetPluginSettingsFromTable();

  // Save plugin config
  config.SetPluginConfig(plugin);

  // Dataset Splits
  SplitConfig split_cfg;
  split_cfg.enabled = ui_->splits_enabled_checkbox_->isChecked();
  split_cfg.target_train_ratio = ui_->train_ratio_slider_->value() / 100.0;
  split_cfg.target_val_ratio = ui_->val_ratio_slider_->value() / 100.0;
  split_cfg.target_test_ratio = ui_->test_ratio_slider_->value() / 100.0;
  split_cfg.hash_salt = ui_->salt_edit_->text();
  config.SetSplitConfig(split_cfg);
}

void AIModelSettingsTab::RefreshModelList()
{
  // Reload model list from current config
  const QList<ModelVersion>& models = config_.GetModelVersions();
  ui_->model_versions_table_->setRowCount(models.size());
  for (int i = 0; i < models.size(); ++i)
  {
    const ModelVersion& model = models[i];
    ui_->model_versions_table_->setItem(i, 0, new QTableWidgetItem(model.name));
    ui_->model_versions_table_->setItem(i, 1,
                                    new QTableWidgetItem(model.timestamp.toString("yyyy-MM-dd")));
    ui_->model_versions_table_->setItem(i, 2,
                                    new QTableWidgetItem(QString::number(model.training_images_count)));
    ui_->model_versions_table_->setItem(i, 3, new QTableWidgetItem(model.path));
    ui_->model_versions_table_->setItem(i, 4, new QTableWidgetItem(model.notes));
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
    ui_->plugin_script_edit_->setText(file);
  }
}

void AIModelSettingsTab::OnAddPluginSetting()
{
  bool ok;
  QString key =
      QInputDialog::getText(this, "Add Setting", "Setting name:", QLineEdit::Normal, "", &ok);
  if (ok && !key.isEmpty())
  {
    // Check for duplicate key
    for (int row = 0; row < ui_->plugin_settings_table_->rowCount(); ++row)
    {
      QTableWidgetItem* key_item = ui_->plugin_settings_table_->item(row, 0);
      if (key_item && key_item->text() == key)
      {
        QMessageBox::warning(this, "Duplicate Setting",
                             QString("Setting '%1' already exists!").arg(key));
        return;
      }
    }

    int row = ui_->plugin_settings_table_->rowCount();
    ui_->plugin_settings_table_->insertRow(row);
    ui_->plugin_settings_table_->setItem(row, 0, new QTableWidgetItem(key));
    ui_->plugin_settings_table_->setItem(row, 1, new QTableWidgetItem(""));
    ui_->plugin_settings_table_->editItem(ui_->plugin_settings_table_->item(row, 1));
  }
}

void AIModelSettingsTab::OnRemovePluginSetting()
{
  int row = ui_->plugin_settings_table_->currentRow();
  if (row >= 0)
  {
    ui_->plugin_settings_table_->removeRow(row);
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
  int row = ui_->model_versions_table_->currentRow();
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
    ui_->model_versions_table_->setItem(row, 4, new QTableWidgetItem(model.notes));

    QMessageBox::information(this, "Notes Updated", "Model notes have been updated.");
  }
}

void AIModelSettingsTab::OnRemoveModelVersion()
{
  int row = ui_->model_versions_table_->currentRow();
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
  ui_->train_ratio_slider_->setEnabled(enabled);
  ui_->val_ratio_slider_->setEnabled(enabled);
  ui_->test_ratio_slider_->setEnabled(enabled);
  ui_->reset_splits_button_->setEnabled(enabled);

  SplitConfig split_cfg = config_.GetSplitConfig();
  split_cfg.enabled = enabled;
  config_.SetSplitConfig(split_cfg);
  UpdateSplitStatistics();
  emit splitsChanged();
}

void AIModelSettingsTab::OnSplitRatioChanged()
{
  // Update labels
  ui_->train_ratio_label_->setText(QString("%1%").arg(ui_->train_ratio_slider_->value()));
  ui_->val_ratio_label_->setText(QString("%1%").arg(ui_->val_ratio_slider_->value()));
  ui_->test_ratio_label_->setText(QString("%1%").arg(ui_->test_ratio_slider_->value()));

  // Update config
  SplitConfig split_cfg = config_.GetSplitConfig();
  split_cfg.target_train_ratio = ui_->train_ratio_slider_->value() / 100.0;
  split_cfg.target_val_ratio = ui_->val_ratio_slider_->value() / 100.0;
  split_cfg.target_test_ratio = ui_->test_ratio_slider_->value() / 100.0;
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
    ui_->split_statistics_label_->setText("Splits disabled");
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

  int target_train = static_cast<int>(ui_->train_ratio_slider_->value());
  int target_val = static_cast<int>(ui_->val_ratio_slider_->value());
  int target_test = static_cast<int>(ui_->test_ratio_slider_->value());

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

  ui_->split_statistics_label_->setText(stats);
}

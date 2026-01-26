#include "customconfigurationpage.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QProcess>
#include <QSet>
#include <QTableWidgetItem>

#include "../pluginwizard.h"
#include "ui_customconfigurationpage.h"

CustomConfigurationPage::CustomConfigurationPage(PluginWizard* wizard)
    : QWizardPage(wizard), ui_(new Ui::CustomConfigurationPage), wizard_(wizard)
{
  ui_->setupUi(this);

  // Configure table header
  ui_->settings_table_->horizontalHeader()->setStretchLastSection(true);

  // Connect signals
  connect(ui_->add_setting_button_, &QPushButton::clicked, this,
          &CustomConfigurationPage::OnAddSetting);
  connect(ui_->remove_setting_button_, &QPushButton::clicked, this,
          &CustomConfigurationPage::OnRemoveSetting);
  connect(ui_->test_button_, &QPushButton::clicked, this,
          &CustomConfigurationPage::OnTestPlugin);
}

CustomConfigurationPage::~CustomConfigurationPage()
{
  delete ui_;
}

void CustomConfigurationPage::initializePage()
{
  // Restore previous values
  ui_->detect_args_edit_->setText(wizard_->GetDetectArgs());
  ui_->train_args_edit_->setText(wizard_->GetTrainArgs());

  PopulateSettingsTable();
}

void CustomConfigurationPage::PopulateSettingsTable()
{
  ui_->settings_table_->setRowCount(0);

  QMap<QString, QString> settings = wizard_->GetCustomSettings();
  for (auto it = settings.begin(); it != settings.end(); ++it)
  {
    int row = ui_->settings_table_->rowCount();
    ui_->settings_table_->insertRow(row);
    ui_->settings_table_->setItem(row, 0, new QTableWidgetItem(it.key()));
    ui_->settings_table_->setItem(row, 1, new QTableWidgetItem(it.value()));
  }

  // Add default settings if empty
  if (ui_->settings_table_->rowCount() == 0)
  {
    ui_->settings_table_->insertRow(0);
    ui_->settings_table_->setItem(0, 0, new QTableWidgetItem("model"));
    ui_->settings_table_->setItem(0, 1, new QTableWidgetItem("/path/to/model.pt"));

    ui_->settings_table_->insertRow(1);
    ui_->settings_table_->setItem(1, 0, new QTableWidgetItem("confidence"));
    ui_->settings_table_->setItem(1, 1, new QTableWidgetItem("0.5"));

    ui_->settings_table_->insertRow(2);
    ui_->settings_table_->setItem(2, 0, new QTableWidgetItem("device"));
    ui_->settings_table_->setItem(2, 1, new QTableWidgetItem("cuda"));
  }
}

QMap<QString, QString> CustomConfigurationPage::GetSettingsFromTable() const
{
  QMap<QString, QString> settings;

  for (int row = 0; row < ui_->settings_table_->rowCount(); ++row)
  {
    QTableWidgetItem* key_item = ui_->settings_table_->item(row, 0);
    QTableWidgetItem* value_item = ui_->settings_table_->item(row, 1);

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

void CustomConfigurationPage::OnAddSetting()
{
  int row = ui_->settings_table_->rowCount();
  ui_->settings_table_->insertRow(row);
  ui_->settings_table_->setItem(row, 0, new QTableWidgetItem(""));
  ui_->settings_table_->setItem(row, 1, new QTableWidgetItem(""));
  ui_->settings_table_->editItem(ui_->settings_table_->item(row, 0));
}

void CustomConfigurationPage::OnRemoveSetting()
{
  QList<QTableWidgetItem*> selected = ui_->settings_table_->selectedItems();
  if (selected.isEmpty())
  {
    return;
  }

  // Get unique rows
  QSet<int> rows;
  for (QTableWidgetItem* item : selected)
  {
    rows.insert(item->row());
  }

  // Remove rows in reverse order
  QList<int> sorted_rows = rows.values();
  std::sort(sorted_rows.begin(), sorted_rows.end(), std::greater<int>());

  for (int row : sorted_rows)
  {
    ui_->settings_table_->removeRow(row);
  }
}

void CustomConfigurationPage::OnTestPlugin()
{
  ui_->test_output_->clear();

  CustomPluginConfig config = wizard_->GetCustomPluginConfig();
  QString command = config.command;

  if (command.isEmpty())
  {
    ui_->test_output_->setPlainText(tr("Error: No plugin command configured."));
    return;
  }

  ui_->test_output_->setPlainText(tr("Testing: %1 --help\n\n").arg(command));

  QProcess process;
  process.setProcessChannelMode(QProcess::MergedChannels);

  // Run with --help flag
  QStringList args;
  QStringList command_parts = command.split(' ', Qt::SkipEmptyParts);

  if (command_parts.isEmpty())
  {
    ui_->test_output_->appendPlainText(tr("Error: Invalid command."));
    return;
  }

  QString program = command_parts.takeFirst();
  args = command_parts;
  args.append("--help");

  process.start(program, args);

  if (!process.waitForStarted(5000))
  {
    ui_->test_output_->appendPlainText(tr("Error: Failed to start plugin.\n%1")
                                      .arg(process.errorString()));
    return;
  }

  if (!process.waitForFinished(10000))
  {
    process.kill();
    ui_->test_output_->appendPlainText(tr("Warning: Plugin did not respond within 10 seconds."));
    return;
  }

  QString output = process.readAll();
  if (output.isEmpty())
  {
    ui_->test_output_->appendPlainText(
        tr("Plugin started successfully but produced no output."));
  }
  else
  {
    // Truncate if too long
    if (output.length() > 1000)
    {
      output = output.left(1000) + "\n... (truncated)";
    }
    ui_->test_output_->appendPlainText(output);
  }

  if (process.exitCode() == 0)
  {
    ui_->test_output_->appendPlainText(tr("\nPlugin test completed successfully."));
  }
  else
  {
    ui_->test_output_->appendPlainText(
        tr("\nPlugin exited with code: %1").arg(process.exitCode()));
  }
}

bool CustomConfigurationPage::validatePage()
{
  // Save values
  wizard_->SetDetectArgs(ui_->detect_args_edit_->text().trimmed());
  wizard_->SetTrainArgs(ui_->train_args_edit_->text().trimmed());
  wizard_->SetCustomSettings(GetSettingsFromTable());

  return true;
}

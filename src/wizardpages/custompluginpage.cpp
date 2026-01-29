#include "custompluginpage.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#include "../pluginwizard.h"
#include "ui_custompluginpage.h"

CustomPluginPage::CustomPluginPage(PluginWizard* wizard)
    : QWizardPage(wizard), wizard_(wizard), ui_(new Ui::CustomPluginPage)
{
  ui_->setupUi(this);

  setTitle(tr("Custom Plugin Configuration"));
  setSubTitle(tr("Configure your custom plugin command and optional dependencies."));

  ConnectSignals();
}

CustomPluginPage::~CustomPluginPage()
{
  delete ui_;
}

void CustomPluginPage::ConnectSignals()
{
  connect(ui_->command_edit_, &QLineEdit::textChanged, this,
          &CustomPluginPage::OnCommandChanged);
  connect(ui_->browse_requirements_button_, &QPushButton::clicked, this,
          &CustomPluginPage::OnBrowseRequirements);
  connect(ui_->clear_requirements_button_, &QPushButton::clicked, this,
          &CustomPluginPage::OnClearRequirements);
}

void CustomPluginPage::initializePage()
{
  CustomPluginConfig config = wizard_->GetCustomPluginConfig();
  ui_->command_edit_->setText(config.command);
  ui_->requirements_edit_->setText(config.requirements_file);
  ui_->env_setup_edit_->setText(config.env_setup);
  ui_->plugin_name_edit_->setText(config.name);
  ui_->use_venv_checkbox_->setChecked(config.use_project_venv);
}

void CustomPluginPage::OnBrowseRequirements()
{
  QString file = QFileDialog::getOpenFileName(this, tr("Select Requirements File"),
                                              wizard_->GetProjectDir(),
                                              tr("Text Files (*.txt);;All Files (*)"));
  if (!file.isEmpty())
  {
    ui_->requirements_edit_->setText(file);
  }
}

void CustomPluginPage::OnClearRequirements()
{
  ui_->requirements_edit_->clear();
}

void CustomPluginPage::OnCommandChanged()
{
  emit completeChanged();
}

bool CustomPluginPage::validatePage()
{
  QString command = ui_->command_edit_->text().trimmed();

  // Command is always required
  if (command.isEmpty())
  {
    QMessageBox::warning(this, tr("Missing Command"),
                         tr("Please specify the plugin command."));
    return false;
  }

  // Check requirements file if specified
  QString requirements = ui_->requirements_edit_->text().trimmed();
  if (!requirements.isEmpty() && !QFile::exists(requirements))
  {
    QMessageBox::warning(
        this, tr("Requirements File Not Found"),
        tr("The requirements file does not exist:\n%1").arg(requirements));
    return false;
  }

  // Save configuration
  CustomPluginConfig config;
  config.command = command;
  config.requirements_file = requirements;
  config.env_setup = ui_->env_setup_edit_->text().trimmed();
  config.name = ui_->plugin_name_edit_->text().trimmed();
  if (config.name.isEmpty())
  {
    config.name = tr("Custom Plugin");
  }
  config.use_project_venv = ui_->use_venv_checkbox_->isChecked();

  wizard_->SetCustomPluginConfig(config);

  return true;
}

bool CustomPluginPage::isComplete() const
{
  return !ui_->command_edit_->text().trimmed().isEmpty();
}

#include "settingsdialog.h"

#include <QMessageBox>

#include "aimodelsettingstab.h"
#include "importexportsettingstab.h"
#include "projectsettingstab.h"
#include "shortcutssettingstab.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(ProjectConfig& config, const QString& project_dir,
                               QWidget* parent)
    : QDialog(parent), ui_(new Ui::SettingsDialog), config_(config), original_config_(config), project_dir_(project_dir)
{
  ui_->setupUi(this);
  SetupTabs();
  ConnectSignals();
  LoadAllTabs();
}

SettingsDialog::~SettingsDialog()
{
  delete ui_;
}

void SettingsDialog::SetupTabs()
{
  // Create all tabs
  auto* projectTab = new ProjectSettingsTab(config_, project_dir_, this);
  auto* aiModelTab = new AIModelSettingsTab(config_, project_dir_, this);
  auto* importExportTab = new ImportExportSettingsTab(config_, this);
  shortcuts_tab_ = new ShortcutsSettingsTab(this);

  // Add tabs to list for polymorphic access
  tabs_ = {projectTab, aiModelTab, importExportTab, shortcuts_tab_};

  // Initialize all tabs using Template Method pattern
  for (auto* tab : tabs_)
  {
    tab->Initialize();
  }

  // Load shortcuts separately (uses QSettings, not ProjectConfig)
  shortcuts_tab_->LoadShortcuts();

  // Add tabs to tab widget
  ui_->tab_widget_->addTab(projectTab, "Project");
  ui_->tab_widget_->addTab(aiModelTab, "AI / Model");
  ui_->tab_widget_->addTab(importExportTab, "Import/Export");
  ui_->tab_widget_->addTab(shortcuts_tab_, "Shortcuts");

  // Connect signals from tabs
  connect(aiModelTab, &AIModelSettingsTab::requestModelRegistration, this,
          &SettingsDialog::RequestModelRegistration);
  connect(aiModelTab, &AIModelSettingsTab::requestPluginWizard, this,
          &SettingsDialog::RequestPluginWizard);
  connect(shortcuts_tab_, &ShortcutsSettingsTab::shortcutsChanged, this,
          &SettingsDialog::ShortcutsChanged);
}

void SettingsDialog::ConnectSignals()
{
  connect(ui_->apply_button_, &QPushButton::clicked, this, &SettingsDialog::OnApply);
  connect(ui_->save_button_, &QPushButton::clicked, this, &SettingsDialog::OnSave);
  // cancel_button_ connection is in .ui file
}

void SettingsDialog::LoadAllTabs()
{
  // Polymorphic call to load configuration into each tab
  for (auto* tab : tabs_)
  {
    tab->LoadFromConfig(config_);
  }
}

void SettingsDialog::SaveAllTabs()
{
  // Polymorphic call to save configuration from each tab
  for (auto* tab : tabs_)
  {
    tab->SaveToConfig(config_);
  }
}

void SettingsDialog::OnApply()
{
  SaveAllTabs();
  shortcuts_tab_->SaveShortcuts();
  QMessageBox::information(this, "Settings Applied",
                           "Settings have been applied to the current session.");
}

void SettingsDialog::OnSave()
{
  SaveAllTabs();
  shortcuts_tab_->SaveShortcuts();
  accept();
}

void SettingsDialog::SetCurrentTab(int index)
{
  if (index >= 0 && index < ui_->tab_widget_->count())
  {
    ui_->tab_widget_->setCurrentIndex(index);
  }
}

int SettingsDialog::GetShortcutsTabIndex() const
{
  return ui_->tab_widget_->indexOf(shortcuts_tab_);
}

void SettingsDialog::RefreshModelList()
{
  // Find the AI/Model tab and refresh it
  for (auto* tab : tabs_)
  {
    AIModelSettingsTab* aiTab = dynamic_cast<AIModelSettingsTab*>(tab);
    if (aiTab)
    {
      aiTab->RefreshModelList();
      break;
    }
  }
}

#include "settingsdialog.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include "aimodelsettingstab.h"
#include "importexportsettingstab.h"
#include "projectsettingstab.h"
#include "shortcutssettingstab.h"

SettingsDialog::SettingsDialog(ProjectConfig& config, const QString& project_dir,
                               QWidget* parent)
    : QDialog(parent), config_(config), original_config_(config), project_dir_(project_dir)
{
  setWindowTitle("Settings");
  setMinimumSize(800, 667);
  SetupUI();
  LoadAllTabs();
}

void SettingsDialog::SetupUI()
{
  QVBoxLayout* main_layout = new QVBoxLayout(this);

  // Create tab widget
  tab_widget_ = new QTabWidget(this);

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
  tab_widget_->addTab(projectTab, "Project");
  tab_widget_->addTab(aiModelTab, "AI / Model");
  tab_widget_->addTab(importExportTab, "Import/Export");
  tab_widget_->addTab(shortcuts_tab_, "Shortcuts");

  // Connect signals from tabs
  connect(aiModelTab, &AIModelSettingsTab::requestModelRegistration, this,
          &SettingsDialog::RequestModelRegistration);
  connect(shortcuts_tab_, &ShortcutsSettingsTab::shortcutsChanged, this,
          &SettingsDialog::ShortcutsChanged);

  main_layout->addWidget(tab_widget_);

  // Dialog buttons
  QHBoxLayout* button_layout = new QHBoxLayout();
  button_layout->addStretch();

  apply_button_ = new QPushButton("Apply", this);
  save_button_ = new QPushButton("Save", this);
  cancel_button_ = new QPushButton("Cancel", this);

  button_layout->addWidget(apply_button_);
  button_layout->addWidget(save_button_);
  button_layout->addWidget(cancel_button_);

  main_layout->addLayout(button_layout);

  // Connect button signals
  connect(apply_button_, &QPushButton::clicked, this, &SettingsDialog::OnApply);
  connect(save_button_, &QPushButton::clicked, this, &SettingsDialog::OnSave);
  connect(cancel_button_, &QPushButton::clicked, this, &QDialog::reject);
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
  if (index >= 0 && index < tab_widget_->count())
  {
    tab_widget_->setCurrentIndex(index);
  }
}

int SettingsDialog::GetShortcutsTabIndex() const
{
  return tab_widget_->indexOf(shortcuts_tab_);
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

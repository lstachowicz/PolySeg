#include "pluginselectionpage.h"

#include "../pluginwizard.h"
#include "ui_pluginselectionpage.h"

PluginSelectionPage::PluginSelectionPage(PluginWizard* wizard)
    : QWizardPage(wizard), wizard_(wizard), ui_(new Ui::PluginSelectionPage)
{
  ui_->setupUi(this);

  setTitle(tr("Choose Plugin Type"));
  setSubTitle(tr("Select the AI framework you want to use for automatic segmentation."));

  SetupButtonGroup();

  // Select Detectron2 by default
  OnPluginSelected(0);
}

PluginSelectionPage::~PluginSelectionPage()
{
  delete ui_;
}

void PluginSelectionPage::SetupButtonGroup()
{
  plugin_group_ = new QButtonGroup(this);
  plugin_group_->addButton(ui_->detectron2_radio_, 0);
  plugin_group_->addButton(ui_->smp_radio_, 1);
  plugin_group_->addButton(ui_->custom_radio_, 2);

  connect(plugin_group_, QOverload<int>::of(&QButtonGroup::idClicked), this,
          &PluginSelectionPage::OnPluginSelected);
}

void PluginSelectionPage::initializePage()
{
  // Restore previous selection if any
  QString current = wizard_->GetSelectedPluginId();
  if (current == "detectron2")
  {
    ui_->detectron2_radio_->setChecked(true);
  }
  else if (current == "smp")
  {
    ui_->smp_radio_->setChecked(true);
  }
  else if (current == "custom")
  {
    ui_->custom_radio_->setChecked(true);
  }
}

void PluginSelectionPage::OnPluginSelected(int id)
{
  switch (id)
  {
    case 0:
      wizard_->SetSelectedPluginId("detectron2");
      break;
    case 1:
      wizard_->SetSelectedPluginId("smp");
      break;
    case 2:
      wizard_->SetSelectedPluginId("custom");
      break;
  }
  emit completeChanged();
}

bool PluginSelectionPage::validatePage()
{
  return !wizard_->GetSelectedPluginId().isEmpty();
}

bool PluginSelectionPage::isComplete() const
{
  return plugin_group_->checkedId() >= 0;
}

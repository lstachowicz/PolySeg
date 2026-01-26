#include "pretrainedmodelpage_smp.h"

#include "ui_pretrainedmodelpage_smp.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#include "../pluginwizard.h"

PretrainedModelPage_SMP::PretrainedModelPage_SMP(PluginWizard* wizard)
    : QWizardPage(wizard), ui_(new Ui::PretrainedModelPage_SMP), wizard_(wizard)
{
  ui_->setupUi(this);

  // Create button group for mode selection
  mode_group_ = new QButtonGroup(this);
  mode_group_->addButton(ui_->pretrainedRadio, 0);
  mode_group_->addButton(ui_->scratchRadio, 1);
  mode_group_->addButton(ui_->existingRadio, 2);

  SetupConnections();

  // Initial visibility
  ui_->pretrainedRadio->setChecked(true);
  UpdateVisibility();
}

PretrainedModelPage_SMP::~PretrainedModelPage_SMP()
{
  delete ui_;
}

void PretrainedModelPage_SMP::SetupConnections()
{
  connect(mode_group_, QOverload<int>::of(&QButtonGroup::idClicked), this,
          &PretrainedModelPage_SMP::OnModeChanged);
  connect(ui_->browseExistingButton, &QPushButton::clicked, this,
          &PretrainedModelPage_SMP::OnBrowseExistingModel);
}

void PretrainedModelPage_SMP::UpdateVisibility()
{
  int mode = mode_group_->checkedId();
  ui_->smpInfoLabel->setVisible(mode == 0);
  ui_->existingModelWidget->setVisible(mode == 2);
}

void PretrainedModelPage_SMP::initializePage()
{
  // SMP uses ImageNet pretrained weights by default
  wizard_->SetSelectedModelId("imagenet_pretrained");
  UpdateVisibility();
}

void PretrainedModelPage_SMP::OnModeChanged(int id)
{
  UpdateVisibility();

  if (id == 0)
  {
    // Pretrained mode - use ImageNet weights
    wizard_->SetSelectedModelId("imagenet_pretrained");
    wizard_->SetModelPath("");
  }
  else if (id == 1)
  {
    // Scratch mode
    wizard_->SetSelectedModelId("scratch");
    wizard_->SetModelPath("");
  }
  else if (id == 2)
  {
    // Existing model mode
    wizard_->SetSelectedModelId("existing");
    wizard_->SetModelPath(ui_->existingModelEdit->text());
  }

  emit completeChanged();
}

void PretrainedModelPage_SMP::OnBrowseExistingModel()
{
  QString file = QFileDialog::getOpenFileName(
      this, tr("Select Model File"), wizard_->GetProjectDir(),
      tr("PyTorch Models (*.pt *.pth *.pkl);;All Files (*)"));

  if (!file.isEmpty())
  {
    ui_->existingModelEdit->setText(file);
    wizard_->SetModelPath(file);
    emit completeChanged();
  }
}

bool PretrainedModelPage_SMP::validatePage()
{
  int mode = mode_group_->checkedId();

  if (mode == 2)
  {
    // Existing model - must have file selected
    QString model_path = ui_->existingModelEdit->text().trimmed();
    if (model_path.isEmpty())
    {
      QMessageBox::warning(this, tr("No Model Selected"),
                           tr("Please select an existing model file."));
      return false;
    }
    if (!QFile::exists(model_path))
    {
      QMessageBox::warning(this, tr("Model File Not Found"),
                           tr("The selected model file does not exist:\n%1").arg(model_path));
      return false;
    }
    wizard_->SetModelPath(model_path);
  }

  return true;
}

bool PretrainedModelPage_SMP::isComplete() const
{
  int mode = mode_group_->checkedId();

  if (mode == 0)
  {
    // Pretrained mode - always complete (ImageNet weights are auto-downloaded)
    return true;
  }
  else if (mode == 1)
  {
    // Scratch mode - always complete
    return true;
  }
  else if (mode == 2)
  {
    // Existing mode - need file
    return !ui_->existingModelEdit->text().trimmed().isEmpty();
  }

  return false;
}

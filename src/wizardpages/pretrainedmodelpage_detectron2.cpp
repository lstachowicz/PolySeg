#include "pretrainedmodelpage_detectron2.h"

#include "ui_pretrainedmodelpage_detectron2.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#include "../pluginwizard.h"

PretrainedModelPage_Detectron2::PretrainedModelPage_Detectron2(PluginWizard* wizard)
    : QWizardPage(wizard), ui_(new Ui::PretrainedModelPage_Detectron2), wizard_(wizard)
{
  ui_->setupUi(this);

  // Create button group for mode selection
  mode_group_ = new QButtonGroup(this);
  mode_group_->addButton(ui_->downloadRadio, 0);
  mode_group_->addButton(ui_->scratchRadio, 1);
  mode_group_->addButton(ui_->existingRadio, 2);

  // Create button group for model selection
  model_group_ = new QButtonGroup(this);
  model_group_->addButton(ui_->cocoRadio, 0);
  model_group_->addButton(ui_->lvisRadio, 1);
  model_group_->addButton(ui_->cityscapesRadio, 2);

  SetupConnections();

  // Initial visibility
  ui_->downloadRadio->setChecked(true);
  UpdateVisibility();
}

PretrainedModelPage_Detectron2::~PretrainedModelPage_Detectron2()
{
  delete ui_;
}

void PretrainedModelPage_Detectron2::SetupConnections()
{
  connect(mode_group_, QOverload<int>::of(&QButtonGroup::idClicked), this,
          &PretrainedModelPage_Detectron2::OnModeChanged);
  connect(model_group_, QOverload<int>::of(&QButtonGroup::idClicked), this,
          &PretrainedModelPage_Detectron2::OnModelSelected);
  connect(ui_->browseExistingButton, &QPushButton::clicked, this,
          &PretrainedModelPage_Detectron2::OnBrowseExistingModel);
}

void PretrainedModelPage_Detectron2::UpdateVisibility()
{
  int mode = mode_group_->checkedId();
  ui_->pretrainedModelsWidget->setVisible(mode == 0);
  ui_->existingModelWidget->setVisible(mode == 2);
}

void PretrainedModelPage_Detectron2::initializePage()
{
  // Set default model ID based on current selections
  QString arch = wizard_->GetSelectedArchitecture();
  QString backbone = wizard_->GetSelectedBackbone();
  wizard_->SetSelectedModelId("coco_" + arch + "_" + backbone);

  UpdateVisibility();
}

void PretrainedModelPage_Detectron2::OnModeChanged(int id)
{
  UpdateVisibility();

  QString arch = wizard_->GetSelectedArchitecture();
  QString backbone = wizard_->GetSelectedBackbone();

  if (id == 0)
  {
    // Download mode - use selected model
    OnModelSelected(model_group_->checkedId());
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

void PretrainedModelPage_Detectron2::OnModelSelected(int id)
{
  QString arch = wizard_->GetSelectedArchitecture();
  QString backbone = wizard_->GetSelectedBackbone();

  QString dataset;
  switch (id)
  {
    case 0:
      dataset = "coco";
      break;
    case 1:
      dataset = "lvis";
      break;
    case 2:
      dataset = "cityscapes";
      break;
    default:
      dataset = "coco";
  }

  wizard_->SetSelectedModelId(dataset + "_" + arch + "_" + backbone);
}

void PretrainedModelPage_Detectron2::OnBrowseExistingModel()
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

bool PretrainedModelPage_Detectron2::validatePage()
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

bool PretrainedModelPage_Detectron2::isComplete() const
{
  int mode = mode_group_->checkedId();

  if (mode == 0)
  {
    // Download mode - need model selected
    return model_group_->checkedId() >= 0;
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

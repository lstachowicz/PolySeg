#include "modelselectionpage_detectron2.h"

#include "ui_modelselectionpage_detectron2.h"

#include "../pluginwizard.h"

ModelSelectionPage_Detectron2::ModelSelectionPage_Detectron2(PluginWizard* wizard)
    : QWizardPage(wizard), ui_(new Ui::ModelSelectionPage_Detectron2), wizard_(wizard)
{
  ui_->setupUi(this);

  // Create button group for architecture radio buttons
  arch_group_ = new QButtonGroup(this);
  arch_group_->addButton(ui_->maskRcnnRadio, 0);
  arch_group_->addButton(ui_->cascadeRadio, 1);

  PopulateBackbones();
  SetupConnections();
}

ModelSelectionPage_Detectron2::~ModelSelectionPage_Detectron2()
{
  delete ui_;
}

void ModelSelectionPage_Detectron2::PopulateBackbones()
{
  ui_->backboneCombo->clear();
  ui_->backboneCombo->addItem(tr("ResNet-50-FPN (Recommended)"), "R_50_FPN");
  ui_->backboneCombo->addItem(tr("ResNet-101-FPN"), "R_101_FPN");
  ui_->backboneCombo->addItem(tr("ResNeXt-101-32x8d-FPN"), "X_101_32x8d_FPN");
}

void ModelSelectionPage_Detectron2::SetupConnections()
{
  connect(arch_group_, QOverload<int>::of(&QButtonGroup::idClicked), this,
          &ModelSelectionPage_Detectron2::OnArchitectureChanged);
  connect(ui_->backboneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelSelectionPage_Detectron2::OnBackboneChanged);
}

void ModelSelectionPage_Detectron2::initializePage()
{
  // Restore previous selections if any
  QString arch = wizard_->GetSelectedArchitecture();
  QString backbone = wizard_->GetSelectedBackbone();

  if (!arch.isEmpty())
  {
    if (arch == "mask_rcnn")
    {
      ui_->maskRcnnRadio->setChecked(true);
    }
    else if (arch == "cascade_mask_rcnn")
    {
      ui_->cascadeRadio->setChecked(true);
    }
  }
  else
  {
    // Default to Mask R-CNN
    ui_->maskRcnnRadio->setChecked(true);
    wizard_->SetSelectedArchitecture("mask_rcnn");
  }

  if (!backbone.isEmpty())
  {
    int index = ui_->backboneCombo->findData(backbone);
    if (index >= 0)
    {
      ui_->backboneCombo->setCurrentIndex(index);
    }
  }
  else
  {
    // Default to first backbone
    ui_->backboneCombo->setCurrentIndex(0);
    wizard_->SetSelectedBackbone(ui_->backboneCombo->currentData().toString());
  }

  UpdateModelEstimates();
}

void ModelSelectionPage_Detectron2::OnArchitectureChanged(int id)
{
  if (id == 0)
  {
    wizard_->SetSelectedArchitecture("mask_rcnn");
  }
  else
  {
    wizard_->SetSelectedArchitecture("cascade_mask_rcnn");
  }
  UpdateModelEstimates();
}

void ModelSelectionPage_Detectron2::OnBackboneChanged(int index)
{
  (void)index;
  wizard_->SetSelectedBackbone(ui_->backboneCombo->currentData().toString());
  UpdateModelEstimates();
}

void ModelSelectionPage_Detectron2::UpdateModelEstimates()
{
  QString backbone = ui_->backboneCombo->currentData().toString();

  int params = 44;
  int fps_gpu = 8;
  double fps_cpu = 1.0;

  if (backbone == "R_101_FPN")
  {
    params = 63;
    fps_gpu = 6;
    fps_cpu = 0.5;
  }
  else if (backbone == "X_101_32x8d_FPN")
  {
    params = 89;
    fps_gpu = 4;
    fps_cpu = 0.3;
  }

  if (ui_->cascadeRadio->isChecked())
  {
    params += 20;
    fps_gpu -= 2;
    fps_cpu *= 0.7;
  }

  QString text = tr("Estimated: %1M params | ~%2 FPS (GPU) | ~%3 FPS (CPU)")
                     .arg(params)
                     .arg(fps_gpu)
                     .arg(fps_cpu, 0, 'f', 1);
  ui_->estimatesLabel->setText(text);
}

bool ModelSelectionPage_Detectron2::validatePage()
{
  if (ui_->maskRcnnRadio->isChecked())
  {
    wizard_->SetSelectedArchitecture("mask_rcnn");
  }
  else if (ui_->cascadeRadio->isChecked())
  {
    wizard_->SetSelectedArchitecture("cascade_mask_rcnn");
  }

  wizard_->SetSelectedBackbone(ui_->backboneCombo->currentData().toString());

  return true;
}

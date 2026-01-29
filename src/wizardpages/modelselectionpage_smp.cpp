#include "modelselectionpage_smp.h"

#include "ui_modelselectionpage_smp.h"

#include "../pluginwizard.h"

ModelSelectionPage_SMP::ModelSelectionPage_SMP(PluginWizard* wizard)
    : QWizardPage(wizard), ui_(new Ui::ModelSelectionPage_SMP), wizard_(wizard)
{
  ui_->setupUi(this);

  PopulateDecoders();
  PopulateEncoders();
  SetupConnections();
}

ModelSelectionPage_SMP::~ModelSelectionPage_SMP()
{
  delete ui_;
}

void ModelSelectionPage_SMP::PopulateDecoders()
{
  ui_->decoderCombo->clear();
  ui_->decoderCombo->addItem(tr("UNet"), "Unet");
  ui_->decoderCombo->addItem(tr("UNet++ (Recommended)"), "UnetPlusPlus");
  ui_->decoderCombo->addItem(tr("DeepLabV3+"), "DeepLabV3Plus");
  ui_->decoderCombo->addItem(tr("FPN"), "FPN");
  ui_->decoderCombo->addItem(tr("PSPNet"), "PSPNet");
  ui_->decoderCombo->addItem(tr("MAnet"), "MAnet");
  ui_->decoderCombo->setCurrentIndex(1);  // UNet++ default
}

void ModelSelectionPage_SMP::PopulateEncoders()
{
  ui_->encoderCombo->clear();
  ui_->encoderCombo->addItem(tr("ResNet-34"), "resnet34");
  ui_->encoderCombo->addItem(tr("ResNet-50"), "resnet50");
  ui_->encoderCombo->addItem(tr("EfficientNet-B0"), "efficientnet-b0");
  ui_->encoderCombo->addItem(tr("EfficientNet-B3 (Recommended)"), "efficientnet-b3");
  ui_->encoderCombo->addItem(tr("EfficientNet-B5"), "efficientnet-b5");
  ui_->encoderCombo->addItem(tr("MobileNet-V2"), "mobilenet_v2");
  ui_->encoderCombo->setCurrentIndex(3);  // EfficientNet-B3 default
}

void ModelSelectionPage_SMP::SetupConnections()
{
  connect(ui_->decoderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelSelectionPage_SMP::UpdateModelEstimates);
  connect(ui_->encoderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelSelectionPage_SMP::UpdateModelEstimates);
}

void ModelSelectionPage_SMP::initializePage()
{
  // Restore previous selections if any
  QString arch = wizard_->GetSelectedArchitecture();
  QString backbone = wizard_->GetSelectedBackbone();

  if (!arch.isEmpty())
  {
    int index = ui_->decoderCombo->findData(arch);
    if (index >= 0)
    {
      ui_->decoderCombo->setCurrentIndex(index);
    }
  }

  if (!backbone.isEmpty())
  {
    int index = ui_->encoderCombo->findData(backbone);
    if (index >= 0)
    {
      ui_->encoderCombo->setCurrentIndex(index);
    }
  }

  UpdateModelEstimates();
}

void ModelSelectionPage_SMP::UpdateModelEstimates()
{
  QString encoder = ui_->encoderCombo->currentData().toString();

  int params = 21;
  int fps_gpu = 25;
  int fps_cpu = 3;

  if (encoder == "resnet50")
  {
    params = 25;
    fps_gpu = 20;
    fps_cpu = 2;
  }
  else if (encoder == "efficientnet-b0")
  {
    params = 5;
    fps_gpu = 40;
    fps_cpu = 5;
  }
  else if (encoder == "efficientnet-b3")
  {
    params = 12;
    fps_gpu = 25;
    fps_cpu = 3;
  }
  else if (encoder == "efficientnet-b5")
  {
    params = 30;
    fps_gpu = 15;
    fps_cpu = 1;
  }
  else if (encoder == "mobilenet_v2")
  {
    params = 3;
    fps_gpu = 50;
    fps_cpu = 8;
  }

  QString text =
      tr("Estimated: %1M params | ~%2 FPS (GPU) | ~%3 FPS (CPU)").arg(params).arg(fps_gpu).arg(fps_cpu);
  ui_->estimatesLabel->setText(text);

  // Save current selections to wizard
  wizard_->SetSelectedArchitecture(ui_->decoderCombo->currentData().toString());
  wizard_->SetSelectedBackbone(encoder);
}

bool ModelSelectionPage_SMP::validatePage()
{
  wizard_->SetSelectedArchitecture(ui_->decoderCombo->currentData().toString());
  wizard_->SetSelectedBackbone(ui_->encoderCombo->currentData().toString());

  return true;
}

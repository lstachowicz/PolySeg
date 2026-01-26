#include "modelselectionpage.h"

#include <QCheckBox>
#include <QFrame>
#include <QGroupBox>
#include <QVBoxLayout>

#include "../pluginwizard.h"
#include "ui_modelselectionpage.h"

ModelSelectionPage::ModelSelectionPage(PluginWizard* wizard)
    : QWizardPage(wizard),
      ui_(new Ui::ModelSelectionPage),
      wizard_(wizard),
      arch_group_(nullptr),
      mask_rcnn_radio_(nullptr),
      cascade_radio_(nullptr),
      backbone_combo_(nullptr),
      decoder_combo_(nullptr),
      encoder_combo_(nullptr),
      pretrained_encoder_checkbox_(nullptr)
{
  ui_->setupUi(this);
  setTitle(tr("Select Model Architecture"));
}

ModelSelectionPage::~ModelSelectionPage()
{
  delete ui_;
}

void ModelSelectionPage::ClearLayout()
{
  if (ui_->content_widget_->layout())
  {
    QLayoutItem* item;
    while ((item = ui_->content_widget_->layout()->takeAt(0)) != nullptr)
    {
      delete item->widget();
      delete item;
    }
    delete ui_->content_widget_->layout();
  }
}

void ModelSelectionPage::SetupDetectron2UI()
{
  ClearLayout();

  setSubTitle(tr("Choose the model architecture and backbone for Detectron2."));

  QVBoxLayout* layout = new QVBoxLayout(ui_->content_widget_);
  layout->setSpacing(15);

  // Architecture selection
  QGroupBox* arch_group_box = new QGroupBox(tr("Architecture"), this);
  QVBoxLayout* arch_layout = new QVBoxLayout(arch_group_box);

  arch_group_ = new QButtonGroup(this);
  connect(arch_group_, QOverload<int>::of(&QButtonGroup::idClicked), this,
          &ModelSelectionPage::OnArchitectureChanged);

  // Mask R-CNN option
  QFrame* mask_rcnn_frame = new QFrame(this);
  mask_rcnn_frame->setFrameStyle(QFrame::StyledPanel);
  QVBoxLayout* mask_rcnn_layout = new QVBoxLayout(mask_rcnn_frame);

  mask_rcnn_radio_ = new QRadioButton(tr("Mask R-CNN (Recommended)"), this);
  QFont bold_font = mask_rcnn_radio_->font();
  bold_font.setBold(true);
  mask_rcnn_radio_->setFont(bold_font);
  mask_rcnn_layout->addWidget(mask_rcnn_radio_);

  QLabel* mask_rcnn_desc =
      new QLabel(tr("Best balance of speed and accuracy\nGood for: General object detection"),
                 this);
  mask_rcnn_desc->setStyleSheet("color: #666; margin-left: 20px;");
  mask_rcnn_layout->addWidget(mask_rcnn_desc);

  arch_group_->addButton(mask_rcnn_radio_, 0);
  arch_layout->addWidget(mask_rcnn_frame);

  // Cascade Mask R-CNN option
  QFrame* cascade_frame = new QFrame(this);
  cascade_frame->setFrameStyle(QFrame::StyledPanel);
  QVBoxLayout* cascade_layout = new QVBoxLayout(cascade_frame);

  cascade_radio_ = new QRadioButton(tr("Cascade Mask R-CNN"), this);
  cascade_layout->addWidget(cascade_radio_);

  QLabel* cascade_desc = new QLabel(
      tr("Higher accuracy, slower inference\nGood for: When accuracy is critical"), this);
  cascade_desc->setStyleSheet("color: #666; margin-left: 20px;");
  cascade_layout->addWidget(cascade_desc);

  arch_group_->addButton(cascade_radio_, 1);
  arch_layout->addWidget(cascade_frame);

  layout->addWidget(arch_group_box);

  // Backbone selection
  QGroupBox* backbone_group = new QGroupBox(tr("Backbone"), this);
  QVBoxLayout* backbone_layout = new QVBoxLayout(backbone_group);

  backbone_combo_ = new QComboBox(this);
  backbone_combo_->addItem(tr("ResNet-50-FPN (Recommended)"), "R_50_FPN");
  backbone_combo_->addItem(tr("ResNet-101-FPN"), "R_101_FPN");
  backbone_combo_->addItem(tr("ResNeXt-101-32x8d-FPN"), "X_101_32x8d_FPN");
  connect(backbone_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelSelectionPage::OnBackboneChanged);
  backbone_layout->addWidget(backbone_combo_);

  QLabel* backbone_info =
      new QLabel(tr("Larger backbones provide better accuracy but are slower"), this);
  backbone_info->setStyleSheet("color: #666;");
  backbone_layout->addWidget(backbone_info);

  layout->addWidget(backbone_group);

  // Select defaults
  mask_rcnn_radio_->setChecked(true);
  backbone_combo_->setCurrentIndex(0);

  OnArchitectureChanged(0);
}

void ModelSelectionPage::SetupSmpUI()
{
  ClearLayout();

  setSubTitle(
      tr("Choose the decoder architecture and encoder backbone for Segmentation Models "
         "PyTorch."));

  QVBoxLayout* layout = new QVBoxLayout(ui_->content_widget_);
  layout->setSpacing(15);

  // Decoder selection
  QGroupBox* decoder_group = new QGroupBox(tr("Decoder (Architecture)"), this);
  QVBoxLayout* decoder_layout = new QVBoxLayout(decoder_group);

  decoder_combo_ = new QComboBox(this);
  decoder_combo_->addItem(tr("UNet"), "Unet");
  decoder_combo_->addItem(tr("UNet++ (Recommended)"), "UnetPlusPlus");
  decoder_combo_->addItem(tr("DeepLabV3+"), "DeepLabV3Plus");
  decoder_combo_->addItem(tr("FPN"), "FPN");
  decoder_combo_->addItem(tr("PSPNet"), "PSPNet");
  decoder_combo_->addItem(tr("MAnet"), "MAnet");
  decoder_combo_->setCurrentIndex(1);  // UNet++ default
  connect(decoder_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelSelectionPage::UpdateModelEstimates);
  decoder_layout->addWidget(decoder_combo_);

  layout->addWidget(decoder_group);

  // Encoder selection
  QGroupBox* encoder_group = new QGroupBox(tr("Encoder (Backbone)"), this);
  QVBoxLayout* encoder_layout = new QVBoxLayout(encoder_group);

  encoder_combo_ = new QComboBox(this);
  encoder_combo_->addItem(tr("ResNet-34"), "resnet34");
  encoder_combo_->addItem(tr("ResNet-50"), "resnet50");
  encoder_combo_->addItem(tr("EfficientNet-B0"), "efficientnet-b0");
  encoder_combo_->addItem(tr("EfficientNet-B3 (Recommended)"), "efficientnet-b3");
  encoder_combo_->addItem(tr("EfficientNet-B5"), "efficientnet-b5");
  encoder_combo_->addItem(tr("MobileNet-V2"), "mobilenet_v2");
  encoder_combo_->setCurrentIndex(3);  // EfficientNet-B3 default
  connect(encoder_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ModelSelectionPage::UpdateModelEstimates);
  encoder_layout->addWidget(encoder_combo_);

  layout->addWidget(encoder_group);

  // Pretrained encoder option
  pretrained_encoder_checkbox_ =
      new QCheckBox(tr("Use ImageNet pretrained encoder weights (Recommended - faster training)"),
                    this);
  pretrained_encoder_checkbox_->setChecked(true);
  layout->addWidget(pretrained_encoder_checkbox_);

  UpdateModelEstimates();
}

void ModelSelectionPage::initializePage()
{
  QString plugin_id = wizard_->GetSelectedPluginId();

  if (plugin_id == "detectron2")
  {
    SetupDetectron2UI();
  }
  else if (plugin_id == "smp")
  {
    SetupSmpUI();
  }

  // Restore previous selections if any
  QString arch = wizard_->GetSelectedArchitecture();
  QString backbone = wizard_->GetSelectedBackbone();

  if (!arch.isEmpty())
  {
    if (plugin_id == "detectron2")
    {
      if (arch == "mask_rcnn" && mask_rcnn_radio_)
      {
        mask_rcnn_radio_->setChecked(true);
      }
      else if (arch == "cascade_mask_rcnn" && cascade_radio_)
      {
        cascade_radio_->setChecked(true);
      }
    }
    else if (plugin_id == "smp" && decoder_combo_)
    {
      int index = decoder_combo_->findData(arch);
      if (index >= 0)
      {
        decoder_combo_->setCurrentIndex(index);
      }
    }
  }

  if (!backbone.isEmpty())
  {
    if (plugin_id == "detectron2" && backbone_combo_)
    {
      int index = backbone_combo_->findData(backbone);
      if (index >= 0)
      {
        backbone_combo_->setCurrentIndex(index);
      }
    }
    else if (plugin_id == "smp" && encoder_combo_)
    {
      int index = encoder_combo_->findData(backbone);
      if (index >= 0)
      {
        encoder_combo_->setCurrentIndex(index);
      }
    }
  }

  UpdateModelEstimates();
}

void ModelSelectionPage::OnArchitectureChanged(int id)
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

void ModelSelectionPage::OnBackboneChanged(int index)
{
  (void)index;
  if (backbone_combo_)
  {
    wizard_->SetSelectedBackbone(backbone_combo_->currentData().toString());
  }
  UpdateModelEstimates();
}

void ModelSelectionPage::UpdateModelEstimates()
{
  QString plugin_id = wizard_->GetSelectedPluginId();
  QString text;

  if (plugin_id == "detectron2")
  {
    QString backbone =
        backbone_combo_ ? backbone_combo_->currentData().toString() : "R_50_FPN";

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

    if (cascade_radio_ && cascade_radio_->isChecked())
    {
      params += 20;
      fps_gpu -= 2;
      fps_cpu *= 0.7;
    }

    text = tr("Estimated: %1M params | ~%2 FPS (GPU) | ~%3 FPS (CPU)")
               .arg(params)
               .arg(fps_gpu)
               .arg(fps_cpu, 0, 'f', 1);
  }
  else if (plugin_id == "smp")
  {
    QString encoder = encoder_combo_ ? encoder_combo_->currentData().toString() : "resnet34";

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

    text = tr("Estimated: %1M params | ~%2 FPS (GPU) | ~%3 FPS (CPU)")
               .arg(params)
               .arg(fps_gpu)
               .arg(fps_cpu);

    // Save encoder selection
    wizard_->SetSelectedBackbone(encoder);

    // Save decoder selection
    if (decoder_combo_)
    {
      wizard_->SetSelectedArchitecture(decoder_combo_->currentData().toString());
    }
  }

  ui_->estimates_label_->setText(text);
}

bool ModelSelectionPage::validatePage()
{
  QString plugin_id = wizard_->GetSelectedPluginId();

  if (plugin_id == "detectron2")
  {
    if (mask_rcnn_radio_ && mask_rcnn_radio_->isChecked())
    {
      wizard_->SetSelectedArchitecture("mask_rcnn");
    }
    else if (cascade_radio_ && cascade_radio_->isChecked())
    {
      wizard_->SetSelectedArchitecture("cascade_mask_rcnn");
    }

    if (backbone_combo_)
    {
      wizard_->SetSelectedBackbone(backbone_combo_->currentData().toString());
    }
  }
  else if (plugin_id == "smp")
  {
    if (decoder_combo_)
    {
      wizard_->SetSelectedArchitecture(decoder_combo_->currentData().toString());
    }
    if (encoder_combo_)
    {
      wizard_->SetSelectedBackbone(encoder_combo_->currentData().toString());
    }
  }

  return true;
}

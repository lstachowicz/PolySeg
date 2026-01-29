#include "pretrainedmodelpage.h"

#include <QFileDialog>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "../pluginwizard.h"
#include "ui_pretrainedmodelpage.h"

PretrainedModelPage::PretrainedModelPage(PluginWizard* wizard)
    : QWizardPage(wizard), ui_(new Ui::PretrainedModelPage), wizard_(wizard)
{
  ui_->setupUi(this);

  setTitle(tr("Pre-trained Model Selection"));
  setSubTitle(tr("Choose how to initialize your model weights."));

  // Setup button groups
  mode_group_ = new QButtonGroup(this);
  mode_group_->addButton(ui_->download_radio_, 0);
  mode_group_->addButton(ui_->scratch_radio_, 1);
  mode_group_->addButton(ui_->existing_radio_, 2);

  connect(mode_group_, QOverload<int>::of(&QButtonGroup::idClicked), this,
          &PretrainedModelPage::OnModeChanged);

  model_group_ = new QButtonGroup(this);
  connect(model_group_, QOverload<int>::of(&QButtonGroup::idClicked), this,
          &PretrainedModelPage::OnModelSelected);

  connect(ui_->browse_existing_button_, &QPushButton::clicked, this,
          &PretrainedModelPage::OnBrowseExistingModel);

  // Set default
  ui_->download_radio_->setChecked(true);
  OnModeChanged(0);
}

PretrainedModelPage::~PretrainedModelPage()
{
  delete ui_;
}

void PretrainedModelPage::PopulatePretrainedModels()
{
  // Clear existing model radio buttons
  QList<QAbstractButton*> buttons = model_group_->buttons();
  for (QAbstractButton* button : buttons)
  {
    model_group_->removeButton(button);
    delete button;
  }

  QLayout* layout = ui_->pretrained_models_widget_->layout();
  // Remove all items except the label
  while (layout->count() > 1)
  {
    QLayoutItem* item = layout->takeAt(1);
    delete item->widget();
    delete item;
  }

  QString plugin_id = wizard_->GetSelectedPluginId();
  QString arch = wizard_->GetSelectedArchitecture();
  QString backbone = wizard_->GetSelectedBackbone();

  int model_index = 0;

  if (plugin_id == "detectron2")
  {
    // COCO model
    QFrame* coco_frame = new QFrame(this);
    coco_frame->setFrameStyle(QFrame::StyledPanel);
    QVBoxLayout* coco_layout = new QVBoxLayout(coco_frame);

    QRadioButton* coco_radio =
        new QRadioButton(tr("COCO Instance Segmentation (Recommended)"), this);
    QFont bold_font = coco_radio->font();
    bold_font.setBold(true);
    coco_radio->setFont(bold_font);
    coco_layout->addWidget(coco_radio);

    QLabel* coco_info = new QLabel(
        tr("80 classes (person, car, dog, etc.)\nSize: ~178 MB\nmAP: 37.2 on COCO val"), this);
    coco_info->setStyleSheet("color: #666; margin-left: 20px;");
    coco_layout->addWidget(coco_info);

    model_group_->addButton(coco_radio, model_index++);
    layout->addWidget(coco_frame);

    // LVIS model
    QFrame* lvis_frame = new QFrame(this);
    lvis_frame->setFrameStyle(QFrame::StyledPanel);
    QVBoxLayout* lvis_layout = new QVBoxLayout(lvis_frame);

    QRadioButton* lvis_radio = new QRadioButton(tr("LVIS Instance Segmentation"), this);
    lvis_layout->addWidget(lvis_radio);

    QLabel* lvis_info = new QLabel(
        tr("1203 classes (more detailed categories)\nSize: ~182 MB\nmAP: 25.6 on LVIS val"),
        this);
    lvis_info->setStyleSheet("color: #666; margin-left: 20px;");
    lvis_layout->addWidget(lvis_info);

    model_group_->addButton(lvis_radio, model_index++);
    layout->addWidget(lvis_frame);

    // Cityscapes model
    QFrame* cityscapes_frame = new QFrame(this);
    cityscapes_frame->setFrameStyle(QFrame::StyledPanel);
    QVBoxLayout* cityscapes_layout = new QVBoxLayout(cityscapes_frame);

    QRadioButton* cityscapes_radio = new QRadioButton(tr("Cityscapes (Urban scenes)"), this);
    cityscapes_layout->addWidget(cityscapes_radio);

    QLabel* cityscapes_info = new QLabel(
        tr("8 classes (car, pedestrian, etc.)\nSize: ~175 MB\nOptimized for street scenes"),
        this);
    cityscapes_info->setStyleSheet("color: #666; margin-left: 20px;");
    cityscapes_layout->addWidget(cityscapes_info);

    model_group_->addButton(cityscapes_radio, model_index++);
    layout->addWidget(cityscapes_frame);

    // Select COCO by default
    if (model_group_->button(0))
    {
      model_group_->button(0)->setChecked(true);
      wizard_->SetSelectedModelId("coco_" + arch + "_" + backbone);
    }
  }
  else if (plugin_id == "smp")
  {
    // SMP uses ImageNet pretrained encoders
    QLabel* smp_info = new QLabel(
        tr("SMP models use ImageNet pre-trained encoder weights.\n"
           "These are automatically downloaded by PyTorch when the model is first loaded.\n\n"
           "No additional model download is required."),
        this);
    smp_info->setWordWrap(true);
    smp_info->setStyleSheet(
        "QLabel { background-color: #e8f4e8; padding: 15px; border-radius: 5px; }");
    layout->addWidget(smp_info);

    // For SMP, we skip the model selection
    wizard_->SetSelectedModelId("imagenet_pretrained");
  }
}

void PretrainedModelPage::initializePage()
{
  PopulatePretrainedModels();

  // Show/hide based on mode
  OnModeChanged(mode_group_->checkedId());
}

void PretrainedModelPage::OnModeChanged(int id)
{
  ui_->pretrained_models_widget_->setVisible(id == 0);
  ui_->existing_model_widget_->setVisible(id == 2);

  if (id == 0)
  {
    // Download mode - use selected model
    if (model_group_->checkedId() >= 0)
    {
      OnModelSelected(model_group_->checkedId());
    }
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
    wizard_->SetModelPath(ui_->existing_model_edit_->text());
  }

  emit completeChanged();
}

void PretrainedModelPage::OnModelSelected(int id)
{
  QString plugin_id = wizard_->GetSelectedPluginId();
  QString arch = wizard_->GetSelectedArchitecture();
  QString backbone = wizard_->GetSelectedBackbone();

  if (plugin_id == "detectron2")
  {
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
}

void PretrainedModelPage::OnBrowseExistingModel()
{
  QString file = QFileDialog::getOpenFileName(
      this, tr("Select Model File"), wizard_->GetProjectDir(),
      tr("PyTorch Models (*.pt *.pth *.pkl);;All Files (*)"));

  if (!file.isEmpty())
  {
    ui_->existing_model_edit_->setText(file);
    wizard_->SetModelPath(file);
    emit completeChanged();
  }
}

bool PretrainedModelPage::validatePage()
{
  int mode = mode_group_->checkedId();

  if (mode == 2)
  {
    // Existing model - must have file selected
    QString model_path = ui_->existing_model_edit_->text().trimmed();
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

bool PretrainedModelPage::isComplete() const
{
  int mode = mode_group_->checkedId();

  if (mode == 0)
  {
    // Download mode - need model selected (or SMP which auto-selects)
    QString plugin_id = wizard_->GetSelectedPluginId();
    if (plugin_id == "smp")
    {
      return true;
    }
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
    return !ui_->existing_model_edit_->text().trimmed().isEmpty();
  }

  return false;
}

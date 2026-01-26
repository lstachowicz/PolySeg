#include "configurationpage.h"

#include "../pluginwizard.h"
#include "ui_configurationpage.h"

ConfigurationPage::ConfigurationPage(PluginWizard* wizard)
    : QWizardPage(wizard), wizard_(wizard), ui_(new Ui::ConfigurationPage)
{
  ui_->setupUi(this);

  setTitle(tr("Detection Configuration"));
  setSubTitle(tr("Configure the detection parameters for optimal results."));

  SetupButtonGroup();
  ConnectSignals();
  UpdateDeviceAvailability();
}

ConfigurationPage::~ConfigurationPage()
{
  delete ui_;
}

void ConfigurationPage::SetupButtonGroup()
{
  device_group_ = new QButtonGroup(this);
  device_group_->addButton(ui_->device_auto_radio_, 0);
  device_group_->addButton(ui_->device_cpu_radio_, 1);
  device_group_->addButton(ui_->device_gpu_radio_, 2);
  device_group_->addButton(ui_->device_mps_radio_, 3);
}

void ConfigurationPage::ConnectSignals()
{
  connect(ui_->confidence_slider_, &QSlider::valueChanged, this,
          &ConfigurationPage::OnConfidenceSliderChanged);
  connect(ui_->nms_slider_, &QSlider::valueChanged, this,
          &ConfigurationPage::OnNmsSliderChanged);
  connect(device_group_, QOverload<int>::of(&QButtonGroup::idClicked), this,
          &ConfigurationPage::OnDeviceChanged);
}

void ConfigurationPage::UpdateDeviceAvailability()
{
  // Check hardware availability
  PluginWizard::PythonInfo python_info = wizard_->GetPythonInfo();
  if (!python_info.has_cuda)
  {
    ui_->device_gpu_radio_->setEnabled(false);
    ui_->device_gpu_radio_->setText(tr("Force GPU (CUDA) - Not available"));
  }
  if (!python_info.has_mps)
  {
    ui_->device_mps_radio_->setEnabled(false);
    ui_->device_mps_radio_->setText(tr("Force Apple Silicon (MPS) - Not available"));
  }
}

void ConfigurationPage::initializePage()
{
  // Restore previous values
  int confidence_val = static_cast<int>(wizard_->GetConfidenceThreshold() * 100);
  ui_->confidence_slider_->setValue(confidence_val);
  OnConfidenceSliderChanged(confidence_val);

  int nms_val = static_cast<int>(wizard_->GetNmsIouThreshold() * 100);
  ui_->nms_slider_->setValue(nms_val);
  OnNmsSliderChanged(nms_val);

  QString device = wizard_->GetDeviceMode();
  if (device == "cpu")
  {
    ui_->device_cpu_radio_->setChecked(true);
  }
  else if (device == "cuda")
  {
    ui_->device_gpu_radio_->setChecked(true);
  }
  else if (device == "mps")
  {
    ui_->device_mps_radio_->setChecked(true);
  }
  else
  {
    ui_->device_auto_radio_->setChecked(true);
  }
}

void ConfigurationPage::OnConfidenceSliderChanged(int value)
{
  double confidence = value / 100.0;
  ui_->confidence_value_label_->setText(QString::number(confidence, 'f', 2));
  wizard_->SetConfidenceThreshold(confidence);
}

void ConfigurationPage::OnNmsSliderChanged(int value)
{
  double nms = value / 100.0;
  ui_->nms_value_label_->setText(QString::number(nms, 'f', 2));
  wizard_->SetNmsIouThreshold(nms);
}

void ConfigurationPage::OnDeviceChanged(int id)
{
  switch (id)
  {
    case 0:
      wizard_->SetDeviceMode("auto");
      break;
    case 1:
      wizard_->SetDeviceMode("cpu");
      break;
    case 2:
      wizard_->SetDeviceMode("cuda");
      break;
    case 3:
      wizard_->SetDeviceMode("mps");
      break;
  }
}

bool ConfigurationPage::validatePage()
{
  // Save all values
  wizard_->SetConfidenceThreshold(ui_->confidence_slider_->value() / 100.0);
  wizard_->SetNmsIouThreshold(ui_->nms_slider_->value() / 100.0);

  // Store advanced settings in custom_settings_
  QMap<QString, QString> settings = wizard_->GetCustomSettings();
  settings["min_size"] = QString::number(ui_->min_size_spin_->value());
  settings["max_detections"] = QString::number(ui_->max_detections_spin_->value());
  settings["image_size"] = QString::number(ui_->image_size_spin_->value());
  wizard_->SetCustomSettings(settings);

  return true;
}

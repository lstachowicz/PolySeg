#include "summarypage.h"

#include "../pluginwizard.h"
#include "ui_summarypage.h"

SummaryPage::SummaryPage(PluginWizard* wizard)
    : QWizardPage(wizard), wizard_(wizard), ui_(new Ui::SummaryPage)
{
  ui_->setupUi(this);

  setTitle(tr("Configuration Summary"));
  setSubTitle(tr("Review your plugin configuration before finishing."));
}

SummaryPage::~SummaryPage()
{
  delete ui_;
}

void SummaryPage::initializePage()
{
  ui_->summary_text_->setHtml(GenerateSummary());
}

QString SummaryPage::GenerateSummary() const
{
  QString html = "<style>"
                 "table { border-collapse: collapse; width: 100%; }"
                 "td { padding: 4px 8px; }"
                 "td:first-child { font-weight: bold; color: #555; width: 40%; }"
                 "</style>";

  html += "<table>";

  QString plugin_id = wizard_->GetSelectedPluginId();

  // Plugin type
  QString plugin_name;
  if (plugin_id == "detectron2")
  {
    plugin_name = "Detectron2";
  }
  else if (plugin_id == "smp")
  {
    plugin_name = "Segmentation Models PyTorch (SMP)";
  }
  else if (plugin_id == "custom")
  {
    CustomPluginConfig config = wizard_->GetCustomPluginConfig();
    plugin_name = config.name.isEmpty() ? tr("Custom Plugin") : config.name;
  }
  html += QString("<tr><td>%1</td><td>%2</td></tr>")
              .arg(tr("Plugin:"))
              .arg(plugin_name);

  if (plugin_id == "custom")
  {
    // Custom plugin specific info
    CustomPluginConfig config = wizard_->GetCustomPluginConfig();
    html += QString("<tr><td>%1</td><td><code>%2</code></td></tr>")
                .arg(tr("Command:"))
                .arg(config.command);

    if (!config.requirements_file.isEmpty())
    {
      html += QString("<tr><td>%1</td><td>%2</td></tr>")
                  .arg(tr("Requirements:"))
                  .arg(config.requirements_file);
    }

    if (!config.env_setup.isEmpty())
    {
      html += QString("<tr><td>%1</td><td><code>%2</code></td></tr>")
                  .arg(tr("Env Setup:"))
                  .arg(config.env_setup);
    }

    QString detect_args = wizard_->GetDetectArgs();
    if (!detect_args.isEmpty())
    {
      html += QString("<tr><td>%1</td><td><code>%2</code></td></tr>")
                  .arg(tr("Detect Args:"))
                  .arg(detect_args);
    }
  }
  else
  {
    // Standard plugin info
    QString arch = wizard_->GetSelectedArchitecture();
    QString backbone = wizard_->GetSelectedBackbone();

    // Format architecture name
    QString arch_display = arch;
    arch_display.replace("_", " ");
    arch_display.replace("mask rcnn", "Mask R-CNN");
    arch_display.replace("cascade mask rcnn", "Cascade Mask R-CNN");

    html += QString("<tr><td>%1</td><td>%2</td></tr>")
                .arg(tr("Architecture:"))
                .arg(arch_display);

    // Format backbone name
    QString backbone_display = backbone;
    backbone_display.replace("_", "-");
    backbone_display.replace("R-50", "ResNet-50");
    backbone_display.replace("R-101", "ResNet-101");
    backbone_display.replace("X-101", "ResNeXt-101");

    html += QString("<tr><td>%1</td><td>%2</td></tr>")
                .arg(tr("Backbone:"))
                .arg(backbone_display);

    // Model info
    QString model_id = wizard_->GetSelectedModelId();
    QString model_display;
    if (model_id == "scratch")
    {
      model_display = tr("Training from scratch");
    }
    else if (model_id == "existing")
    {
      model_display = tr("Existing model: %1").arg(wizard_->GetModelPath());
    }
    else if (model_id == "imagenet_pretrained")
    {
      model_display = tr("ImageNet pretrained encoder");
    }
    else if (model_id.startsWith("coco_"))
    {
      model_display = tr("COCO Instance Segmentation");
    }
    else if (model_id.startsWith("lvis_"))
    {
      model_display = tr("LVIS Instance Segmentation");
    }
    else if (model_id.startsWith("cityscapes_"))
    {
      model_display = tr("Cityscapes");
    }
    else
    {
      model_display = model_id;
    }

    html += QString("<tr><td>%1</td><td>%2</td></tr>")
                .arg(tr("Model:"))
                .arg(model_display);

    // Model path
    QString model_path = wizard_->GetModelPath();
    if (!model_path.isEmpty())
    {
      html += QString("<tr><td>%1</td><td><code>%2</code></td></tr>")
                  .arg(tr("Model Path:"))
                  .arg(model_path);
    }
  }

  html += "</table>";

  // Settings section
  html += "<br><b>" + tr("Settings:") + "</b><ul style='margin-top: 5px;'>";

  if (plugin_id != "custom")
  {
    html += QString("<li>%1 %2</li>")
                .arg(tr("Confidence:"))
                .arg(wizard_->GetConfidenceThreshold(), 0, 'f', 2);

    html += QString("<li>%1 %2</li>")
                .arg(tr("NMS IoU:"))
                .arg(wizard_->GetNmsIouThreshold(), 0, 'f', 2);

    QString device = wizard_->GetDeviceMode();
    QString device_display;
    if (device == "auto")
    {
      PluginWizard::PythonInfo info = wizard_->GetPythonInfo();
      device_display = info.has_cuda ? tr("Auto (GPU detected)") : tr("Auto (CPU)");
    }
    else if (device == "cpu")
    {
      device_display = tr("Force CPU");
    }
    else
    {
      device_display = tr("Force GPU (CUDA)");
    }

    html += QString("<li>%1 %2</li>").arg(tr("Device:")).arg(device_display);
  }

  // Custom settings
  QMap<QString, QString> settings = wizard_->GetCustomSettings();
  for (auto it = settings.begin(); it != settings.end(); ++it)
  {
    html += QString("<li>%1: %2</li>").arg(it.key()).arg(it.value());
  }

  html += "</ul>";

  return html;
}

bool SummaryPage::validatePage()
{
  // TODO: If test_detection_checkbox_ is checked, run a test detection
  // For now, just return true

  return true;
}

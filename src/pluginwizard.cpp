#include "pluginwizard.h"

#include <QDir>
#include <QVBoxLayout>

#include "wizardpages/configurationpage.h"
#include "wizardpages/customconfigurationpage.h"
#include "wizardpages/custompluginpage.h"
#include "wizardpages/downloadpage.h"
#include "wizardpages/modelselectionpage.h"
#include "wizardpages/pluginselectionpage.h"
#include "wizardpages/pretrainedmodelpage.h"
#include "wizardpages/summarypage.h"
#include "wizardpages/welcomepage.h"

PluginWizard::PluginWizard(QWidget* parent, const QString& project_dir,
                           WizardContext context)
    : QWizard(parent), context_(context), project_dir_(project_dir)
{
  setWindowTitle(tr("Plugin Installation Wizard"));
  setWizardStyle(QWizard::ModernStyle);
  setMinimumSize(700, 550);

  // Set options
  setOption(QWizard::NoBackButtonOnStartPage, true);
  setOption(QWizard::HaveHelpButton, false);

  SetupPages();
}

void PluginWizard::SetupPages()
{
  // Create all pages
  welcome_page_ = new WelcomePage(this);
  plugin_selection_page_ = new PluginSelectionPage(this);
  custom_plugin_page_ = new CustomPluginPage(this);
  model_selection_page_ = new ModelSelectionPage(this);
  pretrained_model_page_ = new PretrainedModelPage(this);
  download_page_ = new DownloadPage(this);
  configuration_page_ = new ConfigurationPage(this);
  custom_configuration_page_ = new CustomConfigurationPage(this);
  summary_page_ = new SummaryPage(this);

  // Add pages with IDs
  setPage(Page_Welcome, welcome_page_);
  setPage(Page_PluginSelection, plugin_selection_page_);
  setPage(Page_CustomPlugin, custom_plugin_page_);
  setPage(Page_ModelSelection, model_selection_page_);
  setPage(Page_PretrainedModel, pretrained_model_page_);
  setPage(Page_Download, download_page_);
  setPage(Page_Configuration, configuration_page_);
  setPage(Page_CustomConfiguration, custom_configuration_page_);
  setPage(Page_Summary, summary_page_);

  setStartId(Page_Welcome);
}

int PluginWizard::nextId() const
{
  switch (currentId())
  {
    case Page_Welcome:
      return Page_PluginSelection;

    case Page_PluginSelection:
      // If custom plugin selected, go to custom plugin page
      if (selected_plugin_id_ == "custom")
      {
        return Page_CustomPlugin;
      }
      // Otherwise go to model selection
      return Page_ModelSelection;

    case Page_CustomPlugin:
      // Custom plugins skip model selection and go to custom configuration
      return Page_CustomConfiguration;

    case Page_ModelSelection:
      return Page_PretrainedModel;

    case Page_PretrainedModel:
      // If downloading a model, go to download page
      if (!selected_model_id_.isEmpty() && selected_model_id_ != "scratch" &&
          selected_model_id_ != "existing")
      {
        return Page_Download;
      }
      // Otherwise skip download and go to configuration
      return Page_Configuration;

    case Page_Download:
      return Page_Configuration;

    case Page_Configuration:
      return Page_Summary;

    case Page_CustomConfiguration:
      return Page_Summary;

    case Page_Summary:
      return -1;  // End of wizard

    default:
      return -1;
  }
}

PluginConfig PluginWizard::BuildPluginConfig() const
{
  PluginConfig config;
  config.enabled = true;
  config.plugin_id = selected_plugin_id_;
  config.architecture = selected_architecture_;
  config.backbone = selected_backbone_;
  config.pretrained_model_id = selected_model_id_;
  config.use_project_venv = custom_plugin_config_.use_project_venv;

  // Determine model source
  if (selected_model_id_.isEmpty() || selected_model_id_ == "scratch")
  {
    config.model_source = "scratch";
  }
  else if (selected_model_id_ == "existing")
  {
    config.model_source = "existing";
  }
  else if (selected_model_id_ == "imagenet_pretrained")
  {
    config.model_source = "imagenet";
  }
  else
  {
    config.model_source = "downloaded";
  }

  if (selected_plugin_id_ == "custom")
  {
    // Custom plugin configuration
    config.name = custom_plugin_config_.name.isEmpty() ? "Custom Plugin" : custom_plugin_config_.name;
    config.command = custom_plugin_config_.command;
    config.env_setup = custom_plugin_config_.env_setup;
    config.detect_args = detect_args_;
    config.train_args = train_args_;

    // Copy custom settings
    config.settings = custom_settings_;
  }
  else
  {
    // Standard plugin (detectron2 or smp)
    if (selected_plugin_id_ == "detectron2")
    {
      config.name = "Detectron2";
      config.command = "python3";
      config.script_path = "examples/plugins/detectron2_plugin.py";
    }
    else if (selected_plugin_id_ == "smp")
    {
      config.name = "Segmentation Models PyTorch";
      config.command = "python3";
      config.script_path = "examples/plugins/smp_plugin.py";
    }

    // Build detect_args based on configuration
    config.detect_args = QString("detect --image {image} --model {model} --conf %1")
                             .arg(confidence_threshold_, 0, 'f', 2);

    // Set env_setup for venv if used
    if (config.use_project_venv)
    {
      QString venv_path = QDir(project_dir_).filePath(".venv");
      config.env_setup = QString("source \"%1/bin/activate\"").arg(venv_path);
    }

    // Store settings
    config.settings["confidence"] = QString::number(confidence_threshold_, 'f', 2);
    config.settings["nms_iou"] = QString::number(nms_iou_threshold_, 'f', 2);
    config.settings["device"] = device_mode_;
    config.settings["model"] = model_path_;

    // Merge any custom settings
    for (auto it = custom_settings_.begin(); it != custom_settings_.end(); ++it)
    {
      config.settings[it.key()] = it.value();
    }
  }

  return config;
}

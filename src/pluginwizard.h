#ifndef PLUGINWIZARD_H
#define PLUGINWIZARD_H

#include <QWizard>

#include "projectconfig.h"

class ProjectConfig;
class WelcomePage;
class PluginSelectionPage;
class CustomPluginPage;
class ModelSelectionPage;
class PretrainedModelPage;
class DownloadPage;
class ConfigurationPage;
class CustomConfigurationPage;
class SummaryPage;

/**
 * @brief Wizard context determines which pages are shown and behavior
 */
enum class WizardContext
{
  NewProject,       // Creating new project, includes image loading
  ExistingProject,  // Adding plugin to existing project
  Reconfiguration   // Changing existing plugin configuration
};

/**
 * @brief Page IDs for wizard navigation
 */
enum WizardPageId
{
  Page_Welcome = 0,
  Page_PluginSelection,
  Page_CustomPlugin,
  Page_ModelSelection,
  Page_PretrainedModel,
  Page_Download,
  Page_Configuration,
  Page_CustomConfiguration,
  Page_Summary
};

/**
 * @brief Data structure for plugin information
 */
struct PluginInfo
{
  QString id;
  QString name;
  QString description;
  QString license;
  QStringList strengths;
  QString requirements_file;
  QString script_path;
  bool gpu_recommended;
  bool is_custom;
};

/**
 * @brief Data structure for custom plugin configuration
 */
struct CustomPluginConfig
{
  QString command;
  QString requirements_file;
  QString env_setup;
  QString name;
  bool use_project_venv;
};

/**
 * @brief Data structure for pretrained model information
 */
struct PretrainedModel
{
  QString id;
  QString name;
  QString description;
  QString download_url;
  qint64 size_bytes;
  QString checksum_sha256;
  QString dataset;
  int num_classes;
  QString metrics;
  QString compatible_arch;
};

/**
 * @brief Multi-step wizard for AI plugin installation and configuration
 *
 * Guides the user through:
 * - Plugin type selection (Detectron2/SMP/Custom)
 * - Model architecture selection
 * - Pre-trained model download (optional)
 * - Configuration parameters
 */
class PluginWizard : public QWizard
{
  Q_OBJECT

 public:
  explicit PluginWizard(QWidget* parent, const QString& project_dir,
                        WizardContext context = WizardContext::ExistingProject);
  ~PluginWizard() override = default;

  // Accessors for wizard data
  WizardContext GetContext() const { return context_; }
  QString GetProjectDir() const { return project_dir_; }

  // Selected values (populated as user progresses)
  QString GetSelectedPluginId() const { return selected_plugin_id_; }
  void SetSelectedPluginId(const QString& id) { selected_plugin_id_ = id; }

  QString GetSelectedArchitecture() const { return selected_architecture_; }
  void SetSelectedArchitecture(const QString& arch) { selected_architecture_ = arch; }

  QString GetSelectedBackbone() const { return selected_backbone_; }
  void SetSelectedBackbone(const QString& backbone) { selected_backbone_ = backbone; }

  QString GetSelectedModelId() const { return selected_model_id_; }
  void SetSelectedModelId(const QString& id) { selected_model_id_ = id; }

  CustomPluginConfig GetCustomPluginConfig() const { return custom_plugin_config_; }
  void SetCustomPluginConfig(const CustomPluginConfig& config)
  {
    custom_plugin_config_ = config;
  }

  // Configuration values
  double GetConfidenceThreshold() const { return confidence_threshold_; }
  void SetConfidenceThreshold(double value) { confidence_threshold_ = value; }

  double GetNmsIouThreshold() const { return nms_iou_threshold_; }
  void SetNmsIouThreshold(double value) { nms_iou_threshold_ = value; }

  QString GetDeviceMode() const { return device_mode_; }
  void SetDeviceMode(const QString& mode) { device_mode_ = mode; }

  // Custom plugin args
  QString GetDetectArgs() const { return detect_args_; }
  void SetDetectArgs(const QString& args) { detect_args_ = args; }

  QString GetTrainArgs() const { return train_args_; }
  void SetTrainArgs(const QString& args) { train_args_ = args; }

  QMap<QString, QString> GetCustomSettings() const { return custom_settings_; }
  void SetCustomSettings(const QMap<QString, QString>& settings)
  {
    custom_settings_ = settings;
  }

  // Model path
  QString GetModelPath() const { return model_path_; }
  void SetModelPath(const QString& path) { model_path_ = path; }

  // Python info
  struct PythonInfo
  {
    QString path;
    QString version;
    bool has_venv;
    bool has_pip;
    bool has_cuda;
    QString cuda_version;
    bool has_mps;  // Apple Silicon Metal Performance Shaders
  };

  PythonInfo GetPythonInfo() const { return python_info_; }
  void SetPythonInfo(const PythonInfo& info) { python_info_ = info; }

  /**
   * @brief Build a PluginConfig from the wizard selections
   * @return Configured PluginConfig ready to be saved to project
   */
  PluginConfig BuildPluginConfig() const;

 protected:
  int nextId() const override;

 private:
  void SetupPages();

  WizardContext context_;
  QString project_dir_;

  // Selected values
  QString selected_plugin_id_;
  QString selected_architecture_;
  QString selected_backbone_;
  QString selected_model_id_;
  CustomPluginConfig custom_plugin_config_;

  // Configuration
  double confidence_threshold_ = 0.5;
  double nms_iou_threshold_ = 0.5;
  QString device_mode_ = "auto";
  QString detect_args_;
  QString train_args_;
  QMap<QString, QString> custom_settings_;
  QString model_path_;

  // Python environment
  PythonInfo python_info_;

  // Pages
  WelcomePage* welcome_page_;
  PluginSelectionPage* plugin_selection_page_;
  CustomPluginPage* custom_plugin_page_;
  ModelSelectionPage* model_selection_page_;
  PretrainedModelPage* pretrained_model_page_;
  DownloadPage* download_page_;
  ConfigurationPage* configuration_page_;
  CustomConfigurationPage* custom_configuration_page_;
  SummaryPage* summary_page_;
};

#endif  // PLUGINWIZARD_H

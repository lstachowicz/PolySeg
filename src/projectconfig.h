#ifndef PROJECTCONFIG_H
#define PROJECTCONFIG_H

#include <QColor>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVector>

struct ProjectClass
{
  int id;
  int index;
  QString name;
  QColor color;

  QJsonObject ToJson() const;
  static ProjectClass FromJson(const QJsonObject& json);
};

// Plugin configuration structure
struct PluginConfig
{
  bool enabled;
  QString name;         // Plugin display name (e.g., "AI Detector")
  QString env_setup;    // Environment setup command (e.g., "source venv/bin/activate")
  QString command;      // Command to execute (e.g., "python3")
  QString script_path;  // Path to plugin script (e.g., "./plugins/detector_plugin.py")
  QString detect_args;  // Arguments for detection (e.g., "detect --model {model} --image {image}
                        // --conf {confidence}")
  QString train_args;   // Arguments for training (e.g., "train --data {dataset} --epochs {epochs}")
  QMap<QString, QString> settings;  // Custom plugin settings (model_path, confidence, etc.)

  PluginConfig();
  QJsonObject ToJson() const;
  static PluginConfig FromJson(const QJsonObject& json);
};

// Image crop configuration (for preprocessing during import)
struct CropConfig
{
  bool enabled;
  int x;       // Top-left X coordinate
  int y;       // Top-left Y coordinate
  int width;   // Crop width (0 = use full image width)
  int height;  // Crop height (0 = use full image height)

  CropConfig();
  QJsonObject ToJson() const;
  static CropConfig FromJson(const QJsonObject& json);
};

// Import path configuration for generating file prefixes
struct ImportPathConfig
{
  QString base_path;           // Base path to strip (e.g., "/home/user/Images")
  QStringList skip_folders;    // Folders to skip in remaining path (e.g., ["BMP", "Dane_Surowe"])
  
  ImportPathConfig();
  QJsonObject ToJson() const;
  static ImportPathConfig FromJson(const QJsonObject& json);
};

// Train/Val/Test split configuration
struct SplitConfig
{
  bool enabled;
  double target_train_ratio;
  double target_val_ratio;
  double target_test_ratio;
  QString hash_salt;  // UUID for deterministic assignment

  SplitConfig();
  QJsonObject ToJson() const;
  static SplitConfig FromJson(const QJsonObject& json);
};

// Model version tracking
struct ModelVersion
{
  QString name;
  QString path;
  QDateTime timestamp;
  int training_images_count;
  QString notes;

  ModelVersion();
  QJsonObject ToJson() const;
  static ModelVersion FromJson(const QJsonObject& json);
};

// Annotation type
enum class AnnotationType
{
  Polygon,
  BoundingBox
};

class ProjectConfig
{
 public:
  ProjectConfig();

  bool LoadFromFile(const QString& filepath);
  bool SaveToFile(const QString& filepath);

  void AddClass(const QString& name, const QColor& color, int index = -1);
  void RemoveClass(int class_id);
  void UpdateClass(int class_id, const QString& name, const QColor& color, int index = -1);
  void MoveClass(int class_id, int direction);  // -1 for up, +1 for down
  void ReindexClasses();                        // Auto-update indices based on current order
  void ReorderClasses(const QVector<ProjectClass>& new_order);  // Replace with new order
  ProjectClass* GetClass(int class_id);
  const QVector<ProjectClass>& GetClasses() const { return classes_; }

  QString GetProjectName() const { return project_name_; }
  void SetProjectName(const QString& name) { project_name_ = name; }

  AnnotationType GetAnnotationType() const { return annotation_type_; }
  void SetAnnotationType(AnnotationType type) { annotation_type_ = type; }

  QString GetVersion() const { return version_; }

  // Project directory management
  QString GetProjectDirectory() const { return project_directory_; }
  void SetProjectDirectory(const QString& dir) { project_directory_ = dir; }

  // Plugin management
  PluginConfig& GetPluginConfig() { return plugin_config_; }
  const PluginConfig& GetPluginConfig() const { return plugin_config_; }
  void SetPluginConfig(const PluginConfig& config) { plugin_config_ = config; }
  bool IsPluginEnabled() const { return plugin_config_.enabled; }

  // Statistics
  int GetTotalImages() const { return total_images_; }
  int GetLabeledImages() const { return labeled_images_; }
  int GetTotalPolygons() const { return total_polygons_; }
  void SetTotalImages(int count) { total_images_ = count; }
  void IncrementLabeledImages() { labeled_images_++; }
  void UpdateStatistics(int total, int labeled, int polygons)
  {
    total_images_ = total;
    labeled_images_ = labeled;
    total_polygons_ = polygons;
  }

  // Train/Val/Test Split Management
  SplitConfig& GetSplitConfig() { return split_config_; }
  const SplitConfig& GetSplitConfig() const { return split_config_; }
  void SetSplitConfig(const SplitConfig& config) { split_config_ = config; }
  bool IsSplitEnabled() const { return split_config_.enabled; }

  // Image Crop Management
  CropConfig& GetCropConfig() { return crop_config_; }
  const CropConfig& GetCropConfig() const { return crop_config_; }
  void SetCropConfig(const CropConfig& config) { crop_config_ = config; }
  bool IsCropEnabled() const { return crop_config_.enabled; }

  // Import Path Management
  ImportPathConfig& GetImportPathConfig() { return import_path_config_; }
  const ImportPathConfig& GetImportPathConfig() const { return import_path_config_; }
  void SetImportPathConfig(const ImportPathConfig& config) { import_path_config_ = config; }

  const QMap<QString, QString>& GetImageSplits() const { return image_splits_; }
  QString GetImageSplit(const QString& filename) const;
  void SetImageSplit(const QString& filename, const QString& split);
  void ClearImageSplits() { image_splits_.clear(); }
  void ResetAllSplits();
  QStringList GetImageFiles() const;

  QString DeterministicSplitForImage(const QString& filename) const;
  void UpdateImageSplits(const QStringList& all_images);
  void GenerateSplitFiles(const QString& project_dir);

  int GetTrainCount() const;
  int GetValCount() const;
  int GetTestCount() const;

  // Model Version Management
  const QList<ModelVersion>& GetModelVersions() const { return model_versions_; }
  void AddModelVersion(const ModelVersion& model);
  void RemoveModelVersion(int index);
  void UpdateModelVersion(int index, const ModelVersion& model);

 private:
  QString version_;
  QString project_name_;
  QString project_directory_;
  AnnotationType annotation_type_;
  QVector<ProjectClass> classes_;
  int next_class_id_;
  PluginConfig plugin_config_;

  // Statistics
  int total_images_;
  int labeled_images_;
  int total_polygons_;

  // Image Crop Configuration
  CropConfig crop_config_;

  // Import Path Configuration
  ImportPathConfig import_path_config_;

  // Train/Val/Test Splits
  SplitConfig split_config_;
  QMap<QString, QString> image_splits_;  // filename → "train"/"val"/"test"

  // Model Versions
  QList<ModelVersion> model_versions_;

  QJsonObject ToJson() const;
  void FromJson(const QJsonObject& json);
};

#endif  // PROJECTCONFIG_H

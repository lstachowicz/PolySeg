#include "projectconfig.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QUuid>

#include <iostream>

PluginConfig::PluginConfig()
    : enabled(false),
      name("AI Plugin"),
      env_setup(""),
      command("python3"),
      script_path(""),
      detect_args(""),
      train_args(""),
      plugin_id(""),
      architecture(""),
      backbone(""),
      pretrained_model_id(""),
      model_source(""),
      use_project_venv(false)
{
}

QJsonObject PluginConfig::ToJson() const
{
  QJsonObject obj;
  obj["enabled"] = enabled;
  obj["name"] = name;
  obj["env_setup"] = env_setup;
  obj["command"] = command;
  obj["script_path"] = script_path;
  obj["detect_args"] = detect_args;
  obj["train_args"] = train_args;

  QJsonObject settings_obj;
  for (auto it = settings.begin(); it != settings.end(); ++it)
  {
    settings_obj[it.key()] = it.value();
  }
  obj["settings"] = settings_obj;

  // Wizard-configured fields
  obj["plugin_id"] = plugin_id;
  obj["architecture"] = architecture;
  obj["backbone"] = backbone;
  obj["pretrained_model_id"] = pretrained_model_id;
  obj["model_source"] = model_source;
  obj["use_project_venv"] = use_project_venv;

  return obj;
}

PluginConfig PluginConfig::FromJson(const QJsonObject& json)
{
  PluginConfig pc;
  pc.enabled = json["enabled"].toBool(false);
  pc.name = json["name"].toString("AI Plugin");
  pc.env_setup = json["env_setup"].toString("");
  pc.command = json["command"].toString("python3");
  pc.script_path = json["script_path"].toString("");
  pc.detect_args = json["detect_args"].toString("");
  pc.train_args = json["train_args"].toString("");

  QJsonObject settings_obj = json["settings"].toObject();
  for (auto it = settings_obj.begin(); it != settings_obj.end(); ++it)
  {
    pc.settings[it.key()] = it.value().toString();
  }

  // Set defaults if not present
  if (!pc.settings.contains("base_model"))
  {
    pc.settings["base_model"] = "";
  }
  if (!pc.settings.contains("model"))
  {
    pc.settings["model"] = "";
  }

  // Wizard-configured fields
  pc.plugin_id = json["plugin_id"].toString("");
  pc.architecture = json["architecture"].toString("");
  pc.backbone = json["backbone"].toString("");
  pc.pretrained_model_id = json["pretrained_model_id"].toString("");
  pc.model_source = json["model_source"].toString("");
  pc.use_project_venv = json["use_project_venv"].toBool(false);

  return pc;
}

// CropConfig implementation
CropConfig::CropConfig()
    : enabled(false),
      x(0),
      y(0),
      width(0),
      height(0)
{
}

QJsonObject CropConfig::ToJson() const
{
  QJsonObject obj;
  obj["enabled"] = enabled;
  obj["x"] = x;
  obj["y"] = y;
  obj["width"] = width;
  obj["height"] = height;
  return obj;
}

CropConfig CropConfig::FromJson(const QJsonObject& json)
{
  CropConfig cc;
  cc.enabled = json["enabled"].toBool(false);
  cc.x = json["x"].toInt(0);
  cc.y = json["y"].toInt(0);
  cc.width = json["width"].toInt(0);
  cc.height = json["height"].toInt(0);
  return cc;
}

// ImportPathConfig implementation
ImportPathConfig::ImportPathConfig()
    : base_path(""),
      skip_folders(QStringList() << "BMP" << "Dane_Surowe")
{
}

QJsonObject ImportPathConfig::ToJson() const
{
  QJsonObject obj;
  obj["base_path"] = base_path;
  QJsonArray arr;
  for (const QString& folder : skip_folders)
  {
    arr.append(folder);
  }
  obj["skip_folders"] = arr;
  return obj;
}

ImportPathConfig ImportPathConfig::FromJson(const QJsonObject& json)
{
  ImportPathConfig ipc;
  ipc.base_path = json["base_path"].toString("");
  
  QJsonArray arr = json["skip_folders"].toArray();
  ipc.skip_folders.clear();
  for (const QJsonValue& val : arr)
  {
    ipc.skip_folders.append(val.toString());
  }
  
  // If no skip_folders in JSON, use defaults
  if (ipc.skip_folders.isEmpty())
  {
    ipc.skip_folders << "BMP" << "Dane_Surowe";
  }
  
  return ipc;
}

// SplitConfig implementation
SplitConfig::SplitConfig()
    : enabled(false),
      target_train_ratio(0.7),
      target_val_ratio(0.2),
      target_test_ratio(0.1),
      hash_salt(QUuid::createUuid().toString())
{
}

QJsonObject SplitConfig::ToJson() const
{
  QJsonObject obj;
  obj["enabled"] = enabled;
  obj["target_train_ratio"] = target_train_ratio;
  obj["target_val_ratio"] = target_val_ratio;
  obj["target_test_ratio"] = target_test_ratio;
  obj["hash_salt"] = hash_salt;
  return obj;
}

SplitConfig SplitConfig::FromJson(const QJsonObject& json)
{
  SplitConfig sc;
  sc.enabled = json["enabled"].toBool(false);
  sc.target_train_ratio = json["target_train_ratio"].toDouble(0.7);
  sc.target_val_ratio = json["target_val_ratio"].toDouble(0.2);
  sc.target_test_ratio = json["target_test_ratio"].toDouble(0.1);
  sc.hash_salt = json["hash_salt"].toString(QUuid::createUuid().toString());
  return sc;
}

// ModelVersion implementation
ModelVersion::ModelVersion()
    : name(""),
      path(""),
      timestamp(QDateTime::currentDateTime()),
      training_images_count(0),
      notes("")
{
}

QJsonObject ModelVersion::ToJson() const
{
  QJsonObject obj;
  obj["name"] = name;
  obj["path"] = path;
  obj["timestamp"] = timestamp.toString(Qt::ISODate);
  obj["training_images_count"] = training_images_count;
  obj["notes"] = notes;
  return obj;
}

ModelVersion ModelVersion::FromJson(const QJsonObject& json)
{
  ModelVersion mv;
  mv.name = json["name"].toString();
  mv.path = json["path"].toString();
  mv.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
  mv.training_images_count = json["training_images_count"].toInt(0);
  mv.notes = json["notes"].toString();
  return mv;
}

QJsonObject ProjectClass::ToJson() const
{
  QJsonObject obj;
  obj["id"] = id;
  obj["index"] = index;
  obj["name"] = name;
  obj["color"] = color.name();
  return obj;
}

ProjectClass ProjectClass::FromJson(const QJsonObject& json)
{
  ProjectClass pc;
  pc.id = json["id"].toInt();
  pc.index = json["index"].toInt(0);
  pc.name = json["name"].toString();
  pc.color = QColor(json["color"].toString());
  return pc;
}

ProjectConfig::ProjectConfig()
    : version_("1.0"),
      project_name_("Untitled Project"),
      project_directory_(""),
      annotation_type_(AnnotationType::Polygon),
      next_class_id_(0),
      total_images_(0),
      labeled_images_(0),
      total_polygons_(0)
{
}

bool ProjectConfig::LoadFromFile(const QString& filepath)
{
  QFile file(filepath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    std::cerr << "Failed to open project file: " << filepath.toStdString() << std::endl;
    return false;
  }

  QByteArray data = file.readAll();
  file.close();

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || !doc.isObject())
  {
    std::cerr << "Invalid JSON in project file" << std::endl;
    return false;
  }

  FromJson(doc.object());
  return true;
}

bool ProjectConfig::SaveToFile(const QString& filepath)
{
  QFile file(filepath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    std::cerr << "Failed to save project file: " << filepath.toStdString() << std::endl;
    return false;
  }

  QJsonDocument doc(ToJson());
  file.write(doc.toJson(QJsonDocument::Indented));
  file.close();

  std::cout << "Project saved to: " << filepath.toStdString() << std::endl;
  return true;
}

void ProjectConfig::AddClass(const QString& name, const QColor& color, int index)
{
  ProjectClass pc;
  pc.id = next_class_id_++;
  pc.name = name;
  pc.color = color;
  pc.index = (index >= 0) ? index : classes_.size();
  classes_.push_back(pc);

  std::cout << "Added class: " << name.toStdString() << " (id=" << pc.id << ", index=" << pc.index
            << ", color=" << color.name().toStdString() << ")" << std::endl;
}

void ProjectConfig::RemoveClass(int class_id)
{
  for (int i = 0; i < classes_.size(); ++i)
  {
    if (classes_[i].id == class_id)
    {
      std::cout << "Removed class: " << classes_[i].name.toStdString() << std::endl;
      classes_.removeAt(i);
      return;
    }
  }
}

void ProjectConfig::UpdateClass(int class_id, const QString& name, const QColor& color, int index)
{
  for (auto& pc : classes_)
  {
    if (pc.id == class_id)
    {
      pc.name = name;
      pc.color = color;
      if (index >= 0)
      {
        pc.index = index;
      }
      std::cout << "Updated class id=" << class_id << " to: " << name.toStdString() << std::endl;
      return;
    }
  }
}

ProjectClass* ProjectConfig::GetClass(int class_id)
{
  for (auto& pc : classes_)
  {
    if (pc.id == class_id)
    {
      return &pc;
    }
  }
  return nullptr;
}

void ProjectConfig::MoveClass(int class_id, int direction)
{
  int idx = -1;
  for (int i = 0; i < classes_.size(); ++i)
  {
    if (classes_[i].id == class_id)
    {
      idx = i;
      break;
    }
  }

  if (idx < 0)
  {
    return;  // Class not found
  }

  int new_idx = idx + direction;
  if (new_idx < 0 || new_idx >= classes_.size())
  {
    return;  // Out of bounds
  }

  // Swap classes
  classes_.swapItemsAt(idx, new_idx);

  // Reindex all classes based on new order
  ReindexClasses();
}

void ProjectConfig::ReindexClasses()
{
  for (int i = 0; i < classes_.size(); ++i)
  {
    classes_[i].index = i;
  }
}

void ProjectConfig::ReorderClasses(const QVector<ProjectClass>& new_order)
{
  classes_ = new_order;
  ReindexClasses();
}

QJsonObject ProjectConfig::ToJson() const
{
  QJsonObject obj;
  obj["version"] = version_;
  obj["name"] = project_name_;
  obj["annotation_type"] = (annotation_type_ == AnnotationType::Polygon) ? "polygon" : "boundingbox";
  obj["modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);

  QJsonArray classes_array;
  for (const auto& pc : classes_)
  {
    classes_array.append(pc.ToJson());
  }
  obj["classes"] = classes_array;

  // Plugin configuration
  obj["plugin"] = plugin_config_.ToJson();

  // Statistics
  QJsonObject stats;
  stats["total_images"] = total_images_;
  stats["labeled_images"] = labeled_images_;
  stats["total_polygons"] = total_polygons_;
  obj["statistics"] = stats;

  // Image Crop Configuration
  obj["crop_config"] = crop_config_.ToJson();

  // Import Path Configuration
  obj["import_path_config"] = import_path_config_.ToJson();

  // Train/Val/Test Splits
  obj["split_config"] = split_config_.ToJson();

  QJsonObject splits_obj;
  for (auto it = image_splits_.begin(); it != image_splits_.end(); ++it)
  {
    splits_obj[it.key()] = it.value();
  }
  obj["image_splits"] = splits_obj;

  // Model Versions
  QJsonArray models_array;
  for (const auto& mv : model_versions_)
  {
    models_array.append(mv.ToJson());
  }
  obj["model_versions"] = models_array;
  
  std::cout << "ToJson: Serializing " << model_versions_.size() << " model versions" << std::endl;

  return obj;
}

void ProjectConfig::FromJson(const QJsonObject& json)
{
  version_ = json["version"].toString("1.0");
  project_name_ = json["name"].toString("Untitled Project");

  QString anno_type = json["annotation_type"].toString("polygon");
  annotation_type_ = (anno_type == "boundingbox") ? AnnotationType::BoundingBox : AnnotationType::Polygon;

  classes_.clear();
  QJsonArray classes_array = json["classes"].toArray();
  int max_id = -1;

  for (const auto& class_val : classes_array)
  {
    ProjectClass pc = ProjectClass::FromJson(class_val.toObject());
    classes_.push_back(pc);
    if (pc.id > max_id)
    {
      max_id = pc.id;
    }
  }

  next_class_id_ = max_id + 1;

  // Load plugin configuration
  if (json.contains("plugin"))
  {
    plugin_config_ = PluginConfig::FromJson(json["plugin"].toObject());
    std::cout << "Plugin loaded: " << plugin_config_.name.toStdString()
              << " (enabled=" << (plugin_config_.enabled ? "yes" : "no") << ")" << std::endl;
  }

  // Load statistics
  QJsonObject stats = json["statistics"].toObject();
  total_images_ = stats["total_images"].toInt(0);
  labeled_images_ = stats["labeled_images"].toInt(0);
  total_polygons_ = stats["total_polygons"].toInt(0);

  // Load crop configuration
  if (json.contains("crop_config"))
  {
    crop_config_ = CropConfig::FromJson(json["crop_config"].toObject());
  }

  // Load import path configuration
  if (json.contains("import_path_config"))
  {
    import_path_config_ = ImportPathConfig::FromJson(json["import_path_config"].toObject());
  }

  // Load split configuration
  if (json.contains("split_config"))
  {
    split_config_ = SplitConfig::FromJson(json["split_config"].toObject());
  }

  // Load image splits
  image_splits_.clear();
  QJsonObject splits_obj = json["image_splits"].toObject();
  for (auto it = splits_obj.begin(); it != splits_obj.end(); ++it)
  {
    image_splits_[it.key()] = it.value().toString();
  }

  // Load model versions
  model_versions_.clear();
  QJsonArray models_array = json["model_versions"].toArray();
  for (const auto& model_val : models_array)
  {
    model_versions_.append(ModelVersion::FromJson(model_val.toObject()));
  }
  
  std::cout << "FromJson: Loaded " << model_versions_.size() << " model versions" << std::endl;

  std::cout << "Loaded project: " << project_name_.toStdString() << " with " << classes_.size()
            << " classes" << std::endl;
}

// Train/Val/Test Split Management Implementation

QString ProjectConfig::GetImageSplit(const QString& filename) const
{
  return image_splits_.value(filename, "");
}

void ProjectConfig::SetImageSplit(const QString& filename, const QString& split)
{
  image_splits_[filename] = split;
}

QString ProjectConfig::DeterministicSplitForImage(const QString& filename) const
{
  if (!split_config_.enabled)
  {
    return "";
  }

  // Use MD5 hash of filename + salt for deterministic assignment
  QString input = filename + split_config_.hash_salt;
  QByteArray hash = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Md5);

  // Convert first 4 bytes to integer
  quint32 hash_value = 0;
  for (int i = 0; i < 4 && i < hash.size(); ++i)
  {
    hash_value = (hash_value << 8) | static_cast<quint8>(hash[i]);
  }

  // Map to 0.0-1.0 range
  double normalized = static_cast<double>(hash_value) / static_cast<double>(UINT32_MAX);

  // Assign based on cumulative ratios
  if (normalized < split_config_.target_train_ratio)
  {
    return "train";
  }
  else if (normalized < split_config_.target_train_ratio + split_config_.target_val_ratio)
  {
    return "val";
  }
  else
  {
    return "test";
  }
}

void ProjectConfig::UpdateImageSplits(const QStringList& all_images)
{
  if (!split_config_.enabled)
  {
    return;
  }

  // For existing images: keep their assignments (immutable!)
  // For new images: assign deterministically to maintain target ratios

  for (const QString& img : all_images)
  {
    if (!image_splits_.contains(img))
    {
      // New image - assign deterministically
      QString split = DeterministicSplitForImage(img);
      image_splits_[img] = split;
    }
    // Existing images keep their split assignment
  }

  std::cout << "Updated splits: Train=" << GetTrainCount() << " Val=" << GetValCount()
            << " Test=" << GetTestCount() << std::endl;
}

void ProjectConfig::GenerateSplitFiles(const QString& project_dir)
{
  if (!split_config_.enabled)
  {
    std::cerr << "Splits not enabled" << std::endl;
    return;
  }

  // Create splits directory
  QString splits_dir = project_dir + "/splits";
  QDir dir;
  if (!dir.exists(splits_dir))
  {
    dir.mkpath(splits_dir);
  }

  // Separate images by split
  QStringList train_images, val_images, test_images;
  for (auto it = image_splits_.begin(); it != image_splits_.end(); ++it)
  {
    QString filename = it.key();
    QString split = it.value();
    
    // Use absolute path for plugin compatibility
    QString full_path = project_dir + "/images/" + filename;

    if (split == "train")
    {
      train_images.append(full_path);
    }
    else if (split == "val")
    {
      val_images.append(full_path);
    }
    else if (split == "test")
    {
      test_images.append(full_path);
    }
  }

  // Write train.txt
  QFile train_file(splits_dir + "/train.txt");
  if (train_file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    for (const QString& img : train_images)
    {
      train_file.write((img + "\n").toUtf8());
    }
    train_file.close();
    std::cout << "Generated train.txt with " << train_images.size() << " images" << std::endl;
  }

  // Write val.txt
  QFile val_file(splits_dir + "/val.txt");
  if (val_file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    for (const QString& img : val_images)
    {
      val_file.write((img + "\n").toUtf8());
    }
    val_file.close();
    std::cout << "Generated val.txt with " << val_images.size() << " images" << std::endl;
  }

  // Write test.txt
  QFile test_file(splits_dir + "/test.txt");
  if (test_file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    for (const QString& img : test_images)
    {
      test_file.write((img + "\n").toUtf8());
    }
    test_file.close();
    std::cout << "Generated test.txt with " << test_images.size() << " images" << std::endl;
  }
}

int ProjectConfig::GetTrainCount() const
{
  int count = 0;
  for (const QString& split : image_splits_)
  {
    if (split == "train")
      count++;
  }
  return count;
}

int ProjectConfig::GetValCount() const
{
  int count = 0;
  for (const QString& split : image_splits_)
  {
    if (split == "val")
      count++;
  }
  return count;
}

int ProjectConfig::GetTestCount() const
{
  int count = 0;
  for (const QString& split : image_splits_)
  {
    if (split == "test")
      count++;
  }
  return count;
}

// Model Version Management Implementation

void ProjectConfig::AddModelVersion(const ModelVersion& model)
{
  model_versions_.append(model);
  std::cout << "Added model version: " << model.name.toStdString() 
            << " (path=" << model.path.toStdString() << ")" << std::endl;
  std::cout << "Total models in config: " << model_versions_.size() << std::endl;
}

void ProjectConfig::RemoveModelVersion(int index)
{
  if (index >= 0 && index < model_versions_.size())
  {
    model_versions_.removeAt(index);
  }
}

void ProjectConfig::UpdateModelVersion(int index, const ModelVersion& model)
{
  if (index >= 0 && index < model_versions_.size())
  {
    model_versions_[index] = model;
  }
}

void ProjectConfig::ResetAllSplits()
{
  // Archive old models directory
  if (!project_directory_.isEmpty())
  {
    QString models_dir = project_directory_ + "/models";
    QDir dir(models_dir);
    if (dir.exists() && !dir.isEmpty())
    {
      QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
      QString archive_name = project_directory_ + "/models_old_" + timestamp;
      dir.rename(models_dir, archive_name);
      dir.mkpath(models_dir);  // Create new empty models directory
    }
  }

  // Generate new salt UUID
  split_config_.hash_salt = QUuid::createUuid().toString(QUuid::WithoutBraces);

  // Clear all assignments
  image_splits_.clear();

  // Model versions list will be kept but old models are archived
}

QStringList ProjectConfig::GetImageFiles() const
{
  // Return all images that have split assignments
  return image_splits_.keys();
}

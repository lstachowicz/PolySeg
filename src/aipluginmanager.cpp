#include "aipluginmanager.h"

#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

#include <iostream>

#include "polygoncanvas.h"
#include "projectconfig.h"

AIPluginManager::AIPluginManager(QObject* parent)
    : QObject(parent),
      project_config_(nullptr),
      canvas_(nullptr),
      status_bar_(nullptr),
      image_list_(nullptr),
      training_process_(nullptr)
{
}

AIPluginManager::~AIPluginManager()
{
  if (training_process_ != nullptr)
  {
    training_process_->kill();
    training_process_->deleteLater();
  }
}

void AIPluginManager::SetProjectConfig(ProjectConfig* config)
{
  project_config_ = config;
}

void AIPluginManager::SetCanvas(PolygonCanvas* canvas)
{
  canvas_ = canvas;
}

void AIPluginManager::SetStatusBar(QStatusBar* status_bar)
{
  status_bar_ = status_bar;
}

void AIPluginManager::SetProjectDirectory(const QString& dir)
{
  project_directory_ = dir;
}

void AIPluginManager::SetImageList(const QStringList* list)
{
  image_list_ = list;
}

bool AIPluginManager::IsPluginAvailable() const
{
  if (project_config_ == nullptr)
  {
    return false;
  }

  const PluginConfig& plugin = project_config_->GetPluginConfig();

  if (!plugin.enabled)
  {
    return false;
  }

  if (plugin.script_path.isEmpty())
  {
    return false;
  }

  // Check if script file exists
  QString script = plugin.script_path;
  if (!script.startsWith("/"))
  {
    // Relative path - resolve from project directory
    script = project_directory_ + "/" + script;
  }

  return QFile::exists(script);
}

QString AIPluginManager::BuildPluginCommand(const QString& args_template,
                                            const QMap<QString, QString>& variables) const
{
  QString result = args_template;

  // Replace all {variable} placeholders with actual values
  for (auto it = variables.begin(); it != variables.end(); ++it)
  {
    QString placeholder = "{" + it.key() + "}";
    result.replace(placeholder, it.value());
  }

  return result;
}

void AIPluginManager::ExecutePluginCommand(const QString& command, const QStringList& args)
{
  const PluginConfig& plugin = project_config_->GetPluginConfig();

  QProcess process;
  process.setProcessChannelMode(QProcess::MergedChannels);

  // Set working directory to project directory
  if (!project_directory_.isEmpty())
  {
    process.setWorkingDirectory(project_directory_);
    std::cout << "Working directory: " << project_directory_.toStdString() << std::endl;
  }

  QString full_command = command;
  QStringList full_args = args;

  // If env_setup is provided, wrap the command in a shell with env setup
  if (!plugin.env_setup.isEmpty())
  {
    // Build shell command: env_setup && command args...
    QString shell_command = plugin.env_setup + " && " + command;
    for (const QString& arg : args)
    {
      shell_command += " " + arg;
    }

    full_command = "bash";
    full_args = QStringList() << "-c" << shell_command;

    std::cout << "Executing with env setup: bash -c \"" << shell_command.toStdString() << "\""
              << std::endl;
  }
  else
  {
    std::cout << "Executing: " << command.toStdString();
    for (const QString& arg : args)
    {
      std::cout << " " << arg.toStdString();
    }
    std::cout << std::endl;
  }

  process.start(full_command, full_args);

  if (!process.waitForStarted())
  {
    QMessageBox::critical(nullptr, "Plugin Error", "Failed to start plugin command:\n" + command);
    return;
  }

  // Wait for process to finish (with timeout)
  if (!process.waitForFinished(30000))  // 30 second timeout
  {
    QMessageBox::warning(nullptr, "Plugin Timeout",
                         "Plugin did not respond within 30 seconds.\nProcess terminated.");
    process.kill();
    return;
  }

  QString output = process.readAll();
  int exitCode = process.exitCode();

  std::cout << "Plugin exit code: " << exitCode << std::endl;
  std::cout << "Plugin output:\n" << output.toStdString() << std::endl;

  if (exitCode != 0)
  {
    QMessageBox::critical(
        nullptr, "Plugin Error",
        QString("Plugin exited with error code %1:\n\n%2").arg(exitCode).arg(output));
    return;
  }

  // Parse JSON output for detection results
  ParseDetectionResults(output);
}

void AIPluginManager::ParseDetectionResults(const QString& json_output)
{
  QJsonDocument doc = QJsonDocument::fromJson(json_output.toUtf8());

  if (doc.isNull() || !doc.isObject())
  {
    QMessageBox::warning(nullptr, "Parse Error", "Plugin did not return valid JSON output.");
    return;
  }

  QJsonObject root = doc.object();

  if (!root.contains("detections") || !root["detections"].isArray())
  {
    QMessageBox::warning(nullptr, "Parse Error", "Plugin output missing 'detections' array.");
    return;
  }

  QJsonArray detections = root["detections"].toArray();
  int added = 0;

  // Get image dimensions for coordinate conversion
  QPixmap pixmap = canvas_->GetPixmap();
  if (pixmap.isNull())
  {
    QMessageBox::warning(nullptr, "No Image", "No image loaded.");
    return;
  }

  int img_width = pixmap.width();
  int img_height = pixmap.height();

  // Collect detection details for summary
  QStringList detection_details;

  for (const QJsonValue& det_val : detections)
  {
    QJsonObject det = det_val.toObject();

    QString class_name = det["class"].toString();
    int external_class_id = det["class_id"].toInt(-1);
    double confidence = det["confidence"].toDouble(0.0);
    QJsonArray points_array = det["points"].toArray();

    if (points_array.isEmpty())
    {
      continue;
    }

    // Find or create class
    int class_id = -1;
    QColor class_color = Qt::red;
    QString resolved_class_name;

    // First try to match by class name
    if (!class_name.isEmpty())
    {
      for (const auto& cls : project_config_->GetClasses())
      {
        if (cls.name == class_name)
        {
          class_id = cls.id;
          class_color = cls.color;
          resolved_class_name = cls.name;
          break;
        }
      }
    }

    // If no match by name and we have class_id, try to map by index
    if (class_id == -1 && external_class_id >= 0)
    {
      const auto& classes = project_config_->GetClasses();
      if (external_class_id < classes.size())
      {
        class_id = classes[external_class_id].id;
        class_color = classes[external_class_id].color;
        resolved_class_name = classes[external_class_id].name;
        std::cout << "Mapped external class_id " << external_class_id
                  << " to project class: " << classes[external_class_id].name.toStdString()
                  << std::endl;
      }
    }

    if (class_id == -1)
    {
      // Auto-create class with random color
      QString new_class_name =
          class_name.isEmpty()
              ? QString("Class_%1")
                    .arg(external_class_id >= 0 ? external_class_id
                                                : project_config_->GetClasses().size())
              : class_name;

      QColor color =
          QColor::fromHsv((project_config_->GetClasses().size() * 137) % 360, 200, 200);
      project_config_->AddClass(new_class_name, color);
      class_id = project_config_->GetClasses().last().id;
      class_color = color;
      resolved_class_name = new_class_name;
      emit ClassesUpdated();
      std::cout << "Auto-created class: " << new_class_name.toStdString() << std::endl;
    }

    // Convert normalized coordinates to pixel coordinates
    // Support both formats:
    // 1. Array of pairs: [[x1, y1], [x2, y2], ...]
    // 2. Flat array: [x1, y1, x2, y2, ...]
    QVector<QPoint> polygon_points;

    bool is_flat_array = false;
    if (points_array.size() > 0 && points_array[0].isDouble())
    {
      is_flat_array = true;
    }

    if (is_flat_array)
    {
      // Flat array format: [x1, y1, x2, y2, ...]
      for (int i = 0; i + 1 < points_array.size(); i += 2)
      {
        double x_norm = points_array[i].toDouble();
        double y_norm = points_array[i + 1].toDouble();
        int x = static_cast<int>(x_norm * img_width);
        int y = static_cast<int>(y_norm * img_height);
        polygon_points.append(QPoint(x, y));
      }
    }
    else
    {
      // Array of pairs format: [[x1, y1], [x2, y2], ...]
      for (const QJsonValue& point_val : points_array)
      {
        QJsonArray point = point_val.toArray();
        if (point.size() >= 2)
        {
          double x_norm = point[0].toDouble();
          double y_norm = point[1].toDouble();
          int x = static_cast<int>(x_norm * img_width);
          int y = static_cast<int>(y_norm * img_height);
          polygon_points.append(QPoint(x, y));
        }
      }
    }

    if (polygon_points.size() >= 3)
    {
      canvas_->AddPolygonFromPlugin(polygon_points, class_id, class_color);
      added++;

      // Collect detection info for summary
      int num_points = polygon_points.size();
      QString detail = QString("  #%1: class=\"%2\" (id=%3), confidence=%4%, points=%5")
                           .arg(added)
                           .arg(resolved_class_name)
                           .arg(external_class_id)
                           .arg(confidence * 100.0, 0, 'f', 1)
                           .arg(num_points);
      detection_details.append(detail);
    }
  }

  // Build detailed summary message
  QString summary;
  if (added > 0)
  {
    summary = QString(
                  "Detection successful!\n\n"
                  "Added %1 detection(s):\n%2\n\n"
                  "Review and adjust as needed.")
                  .arg(added)
                  .arg(detection_details.join("\n"));
  }
  else
  {
    summary =
        "No detections were added.\n\n"
        "The plugin ran but no valid polygons were found.";
  }

  QMessageBox::information(nullptr, "Detection Complete", summary);

  emit StatusMessage(QString("Plugin detected %1 objects").arg(added), 5000);
  emit DetectionComplete(added);
}

void AIPluginManager::RunAutoDetect(const QString& current_image_path)
{
  if (!IsPluginAvailable())
  {
    QMessageBox::warning(nullptr, "Plugin Not Available",
                         "AI plugin is not configured or script not found.\n\n"
                         "Go to Edit -> Project Settings -> Plugin Configuration to set it up.");
    return;
  }

  if (current_image_path.isEmpty())
  {
    QMessageBox::warning(nullptr, "No Image", "Please load an image first.");
    return;
  }

  const PluginConfig& plugin = project_config_->GetPluginConfig();

  // Build variable substitutions
  QMap<QString, QString> vars;
  vars["image"] = current_image_path;
  vars["project"] = project_directory_;

  // Add all plugin settings as variables
  for (auto it = plugin.settings.begin(); it != plugin.settings.end(); ++it)
  {
    vars[it.key()] = it.value();
  }

  // Build command arguments
  QString args_string = BuildPluginCommand(plugin.detect_args, vars);
  QStringList args;

  // Resolve script path
  QString script = plugin.script_path;
  if (!script.startsWith("/"))
  {
    script = project_directory_ + "/" + script;
  }
  args.append(script);

  // Add parsed arguments
  args.append(args_string.split(" ", Qt::SkipEmptyParts));

  ExecutePluginCommand(plugin.command, args);
}

void AIPluginManager::RunTrainModel()
{
  if (!IsPluginAvailable())
  {
    QMessageBox::warning(nullptr, "Plugin Not Available",
                         "AI plugin is not configured or script not found.\n\n"
                         "Go to Edit -> Project Settings -> Plugin Configuration to set it up.");
    return;
  }

  if (project_directory_.isEmpty())
  {
    QMessageBox::warning(nullptr, "No Project", "Please open a project first.");
    return;
  }

  // Generate split files if enabled
  if (project_config_->IsSplitEnabled())
  {
    project_config_->GenerateSplitFiles(project_directory_);
  }

  // Remove old cache files to force plugin to rescan labels
  QStringList cache_files;
  cache_files << project_directory_ + "/images.cache" << project_directory_ + "/train.cache"
              << project_directory_ + "/val.cache" << project_directory_ + "/test.cache";

  for (const QString& cache_file : cache_files)
  {
    if (QFile::exists(cache_file))
    {
      if (QFile::remove(cache_file))
      {
        std::cout << "Removed old cache: " << cache_file.toStdString() << std::endl;
      }
    }
  }

  // Generate data.yaml file
  QString data_yaml_path = project_directory_ + "/data.yaml";
  QFile data_yaml(data_yaml_path);
  if (data_yaml.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream out(&data_yaml);
    out << "# Dataset Configuration\n";
    out << "path: " << project_directory_ << "\n";
    out << "train: splits/train.txt\n";
    out << "val: splits/val.txt\n";
    out << "test: splits/test.txt\n\n";
    out << "# Classes\n";
    out << "names:\n";

    const auto& classes = project_config_->GetClasses();
    for (const auto& cls : classes)
    {
      out << "  " << cls.id << ": " << cls.name << "\n";
    }

    out << "\n# Number of classes\n";
    out << "nc: " << classes.size() << "\n";

    data_yaml.close();
    std::cout << "Generated data.yaml with " << classes.size() << " classes" << std::endl;
  }
  else
  {
    QMessageBox::warning(nullptr, "Warning", "Could not create data.yaml file.");
  }

  const PluginConfig& plugin = project_config_->GetPluginConfig();

  // Build variable substitutions
  QMap<QString, QString> vars;

  // Add all plugin settings as variables first
  for (auto it = plugin.settings.begin(); it != plugin.settings.end(); ++it)
  {
    vars[it.key()] = it.value();
  }

  // Override with system variables (these take precedence)
  vars["project"] = project_directory_;
  vars["dataset"] = project_directory_;  // Same as project for now
  vars["splits"] = project_directory_ + "/splits";
  vars["data_yaml"] = project_directory_ + "/data.yaml";
  vars["train_count"] = QString::number(project_config_->GetTrainCount());
  vars["val_count"] = QString::number(project_config_->GetValCount());
  vars["test_count"] = QString::number(project_config_->GetTestCount());

  // Build command arguments
  QString args_string = BuildPluginCommand(plugin.train_args, vars);
  QStringList args;

  // Resolve script path
  QString script = plugin.script_path;
  if (!script.startsWith("/"))
  {
    script = project_directory_ + "/" + script;
  }
  args.append(script);

  // Add parsed arguments
  args.append(args_string.split(" ", Qt::SkipEmptyParts));

  QMessageBox::information(nullptr, "Training Started",
                           "Training will run in the background.\n\n"
                           "Check the terminal for progress.\n\n"
                           "You will be prompted to register the model when training completes.");

  // Clean up any existing training process
  if (training_process_ != nullptr)
  {
    training_process_->kill();
    training_process_->deleteLater();
  }

  // Create tmp directory for logs
  QString tmp_dir = project_directory_ + "/tmp";
  QDir().mkpath(tmp_dir);

  // Create log file paths
  QString log_file_path = tmp_dir + "/training_output.log";
  QString err_file_path = tmp_dir + "/training_error.log";

  // Create process for async execution
  training_process_ = new QProcess(this);
  training_process_->setProcessChannelMode(QProcess::SeparateChannels);

  // Set working directory to project directory
  training_process_->setWorkingDirectory(project_directory_);
  std::cout << "Training working directory: " << project_directory_.toStdString() << std::endl;
  std::cout << "Training logs: " << log_file_path.toStdString() << std::endl;

  // Open log files for writing
  QFile* log_file = new QFile(log_file_path, training_process_);
  QFile* err_file = new QFile(err_file_path, training_process_);

  if (!log_file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
  {
    std::cerr << "Warning: Could not open log file for writing" << std::endl;
    delete log_file;
    log_file = nullptr;
  }

  if (!err_file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
  {
    std::cerr << "Warning: Could not open error log file for writing" << std::endl;
    delete err_file;
    err_file = nullptr;
  }

  // Connect to read output in real-time and write to both terminal and log file
  connect(training_process_, &QProcess::readyReadStandardOutput, this, [this, log_file]() {
    QString output = training_process_->readAllStandardOutput();
    std::cout << output.toStdString() << std::flush;

    if (log_file && log_file->isOpen())
    {
      log_file->write(output.toUtf8());
      log_file->flush();
    }
  });

  connect(training_process_, &QProcess::readyReadStandardError, this, [this, err_file]() {
    QString error_output = training_process_->readAllStandardError();
    std::cerr << error_output.toStdString() << std::flush;

    if (err_file && err_file->isOpen())
    {
      err_file->write(error_output.toUtf8());
      err_file->flush();
    }
  });

  // Connect to finished signal for model registration
  connect(training_process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [this, log_file, err_file](int exitCode, QProcess::ExitStatus exitStatus) {
            // Close log files
            if (log_file && log_file->isOpen())
            {
              log_file->close();
            }
            if (err_file && err_file->isOpen())
            {
              err_file->close();
            }

            if (exitStatus == QProcess::NormalExit && exitCode == 0)
            {
              std::cout << "\n=== Training completed successfully ===\n" << std::endl;
              // Training succeeded - prompt for model registration
              PromptModelRegistration();
              emit TrainingComplete(true);
            }
            else if (exitStatus == QProcess::CrashExit)
            {
              std::cerr << "\n=== Training process crashed ===\n" << std::endl;
              QMessageBox::critical(nullptr, "Training Failed", "Training process crashed.");
              emit TrainingComplete(false);
            }
            else
            {
              std::cerr << "\n=== Training failed with exit code " << exitCode << " ===\n"
                        << std::endl;
              QMessageBox::critical(
                  nullptr, "Training Failed",
                  QString("Training exited with error code %1\n\nCheck terminal for details.")
                      .arg(exitCode));
              emit TrainingComplete(false);
            }

            training_process_->deleteLater();
            training_process_ = nullptr;
          });

  // Start process
  QString full_command = plugin.command;
  QStringList full_args = args;

  // If env_setup is provided, wrap the command in a shell with env setup
  if (!plugin.env_setup.isEmpty())
  {
    // Build shell command: env_setup && command args...
    QString shell_command = plugin.env_setup + " && " + plugin.command;
    for (const QString& arg : args)
    {
      shell_command += " " + arg;
    }

    full_command = "bash";
    full_args = QStringList() << "-c" << shell_command;

    std::cout << "Executing training with env setup: bash -c \"" << shell_command.toStdString()
              << "\"" << std::endl;
  }
  else
  {
    std::cout << "Executing: " << plugin.command.toStdString();
    for (const QString& arg : args)
    {
      std::cout << " " << arg.toStdString();
    }
    std::cout << std::endl;
  }

  training_process_->start(full_command, full_args);

  if (!training_process_->waitForStarted())
  {
    QMessageBox::critical(nullptr, "Plugin Error",
                          "Failed to start training command:\n" + plugin.command);
    training_process_->deleteLater();
    training_process_ = nullptr;
    return;
  }

  std::cout << "\n=== Training started ===\n" << std::endl;
}

void AIPluginManager::RunBatchDetect()
{
  if (!IsPluginAvailable())
  {
    QMessageBox::warning(nullptr, "Plugin Not Available",
                         "AI plugin is not configured or script not found.\n\n"
                         "Go to Edit -> Project Settings -> Plugin Configuration to set it up.");
    return;
  }

  if (project_directory_.isEmpty() || image_list_ == nullptr || image_list_->isEmpty())
  {
    QMessageBox::warning(nullptr, "No Project", "Please open a project with images first.");
    return;
  }

  // Confirm batch operation
  int total_images = image_list_->size();
  int unreviewed = CountUnreviewedImages();

  QString message = QString(
                        "Run AI detection on all %1 images in this project?\n\n"
                        "Results will be saved as .meta files for review.\n"
                        "You can approve/reject each detection individually.\n\n"
                        "Images already reviewed: %2\n"
                        "Images to process: %3")
                        .arg(total_images)
                        .arg(total_images - unreviewed)
                        .arg(unreviewed);

  QMessageBox::StandardButton reply =
      QMessageBox::question(nullptr, "Batch Detection", message, QMessageBox::Yes | QMessageBox::No);

  if (reply != QMessageBox::Yes)
  {
    return;
  }

  // Progress tracking
  int processed = 0;
  int detected = 0;
  int skipped = 0;

  emit StatusMessage("Batch detection in progress...");

  // Process each image
  for (const QString& image_file : *image_list_)
  {
    QString image_path = project_directory_ + "/images/" + image_file;

    // Skip if already has approved annotations (unless user wants to override)
    if (HasApprovedFile(image_path))
    {
      skipped++;
      std::cout << "Skipping (already approved): " << image_file.toStdString() << std::endl;
      continue;
    }

    // Run detection on this image
    BatchDetectOnImage(image_path);
    processed++;

    // Check if meta file was created (detection found something)
    if (HasMetaFile(image_path))
    {
      detected++;
    }

    // Update progress
    emit StatusMessage(QString("Batch detection: %1/%2 processed, %3 detected")
                           .arg(processed)
                           .arg(total_images)
                           .arg(detected));

    // Allow UI updates
    QCoreApplication::processEvents();
  }

  // Show summary
  QString summary = QString(
                        "Batch detection complete!\n\n"
                        "Processed: %1 images\n"
                        "Detections found: %2 images\n"
                        "Skipped (approved): %3 images\n\n"
                        "Use Tools -> Next Unreviewed to review detections.")
                        .arg(processed)
                        .arg(detected)
                        .arg(skipped);

  QMessageBox::information(nullptr, "Batch Detection Complete", summary);

  emit StatusMessage(
      QString("Batch detection complete: %1 detected, %2 skipped").arg(detected).arg(skipped),
      10000);

  // Signal to jump to first unreviewed image
  emit RequestNextUnreviewed();
}

void AIPluginManager::BatchDetectOnImage(const QString& image_path)
{
  const PluginConfig& plugin = project_config_->GetPluginConfig();

  // Build variable substitutions
  QMap<QString, QString> vars;
  vars["image"] = image_path;
  vars["project"] = project_directory_;

  // Add all plugin settings as variables
  for (auto it = plugin.settings.begin(); it != plugin.settings.end(); ++it)
  {
    vars[it.key()] = it.value();
  }

  // Build command arguments
  QString args_string = BuildPluginCommand(plugin.detect_args, vars);
  QStringList args;

  // Resolve script path
  QString script = plugin.script_path;
  if (!script.startsWith("/"))
  {
    script = project_directory_ + "/" + script;
  }
  args.append(script);
  args.append(args_string.split(" ", Qt::SkipEmptyParts));

  // Execute plugin
  QProcess process;
  process.setProcessChannelMode(QProcess::MergedChannels);

  std::cout << "Batch detect: " << image_path.toStdString() << std::endl;

  QString full_command = plugin.command;
  QStringList full_args = args;

  // If env_setup is provided, wrap the command in a shell with env setup
  if (!plugin.env_setup.isEmpty())
  {
    // Build shell command: env_setup && command args...
    QString shell_command = plugin.env_setup + " && " + plugin.command;
    for (const QString& arg : args)
    {
      shell_command += " " + arg;
    }

    full_command = "bash";
    full_args = QStringList() << "-c" << shell_command;
  }

  process.start(full_command, full_args);

  if (!process.waitForStarted())
  {
    std::cerr << "Failed to start plugin for: " << image_path.toStdString() << std::endl;
    return;
  }

  if (!process.waitForFinished(30000))
  {
    std::cerr << "Plugin timeout for: " << image_path.toStdString() << std::endl;
    process.kill();
    return;
  }

  QString output = process.readAll();
  int exitCode = process.exitCode();

  if (exitCode != 0)
  {
    std::cerr << "Plugin error for: " << image_path.toStdString() << " (exit code: " << exitCode
              << ")" << std::endl;
    return;
  }

  // Parse JSON output
  QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
  if (doc.isNull() || !doc.isObject())
  {
    std::cerr << "Invalid JSON from plugin for: " << image_path.toStdString() << std::endl;
    return;
  }

  QJsonObject root = doc.object();
  if (!root.contains("detections") || !root["detections"].isArray())
  {
    std::cerr << "Missing detections array for: " << image_path.toStdString() << std::endl;
    return;
  }

  QJsonArray detections = root["detections"].toArray();

  if (detections.isEmpty())
  {
    std::cout << "No detections for: " << image_path.toStdString() << std::endl;
    return;
  }

  // Save detections to .meta file (temporary, unreviewed)
  QFileInfo fileInfo(image_path);
  QString meta_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".meta";

  QFile meta_file(meta_path);
  if (!meta_file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    std::cerr << "Failed to create meta file: " << meta_path.toStdString() << std::endl;
    return;
  }

  QTextStream out(&meta_file);

  // Load image to get dimensions (for validation, dimensions already in JSON as normalized)
  QPixmap pixmap(image_path);
  if (pixmap.isNull())
  {
    std::cerr << "Failed to load image: " << image_path.toStdString() << std::endl;
    return;
  }

  // Write detections in normalized format to .meta
  for (const QJsonValue& det_val : detections)
  {
    QJsonObject det = det_val.toObject();
    QString class_name = det["class"].toString();
    QJsonArray points_array = det["points"].toArray();

    if (points_array.isEmpty())
    {
      continue;
    }

    // Find or create class
    int class_id = -1;
    for (const auto& cls : project_config_->GetClasses())
    {
      if (cls.name == class_name)
      {
        class_id = cls.id;
        break;
      }
    }

    if (class_id == -1)
    {
      // Auto-create class
      QColor color =
          QColor::fromHsv((project_config_->GetClasses().size() * 137) % 360, 200, 200);
      project_config_->AddClass(class_name, color);
      class_id = project_config_->GetClasses().last().id;
      emit ClassesUpdated();
    }

    // Write polygon in normalized format
    // Support both formats:
    // 1. Array of pairs: [[x1, y1], [x2, y2], ...]
    // 2. Flat array: [x1, y1, x2, y2, ...]
    out << class_id;

    bool is_flat_array = false;
    if (points_array.size() > 0 && points_array[0].isDouble())
    {
      is_flat_array = true;
    }

    if (is_flat_array)
    {
      // Flat array format: [x1, y1, x2, y2, ...]
      for (int i = 0; i + 1 < points_array.size(); i += 2)
      {
        double x_norm = points_array[i].toDouble();
        double y_norm = points_array[i + 1].toDouble();
        out << " " << x_norm << " " << y_norm;
      }
    }
    else
    {
      // Array of pairs format: [[x1, y1], [x2, y2], ...]
      for (const QJsonValue& point_val : points_array)
      {
        QJsonArray point = point_val.toArray();
        if (point.size() >= 2)
        {
          double x_norm = point[0].toDouble();
          double y_norm = point[1].toDouble();
          out << " " << x_norm << " " << y_norm;
        }
      }
    }
    out << "\n";
  }

  meta_file.close();
  std::cout << "Saved " << detections.size() << " detections to: " << meta_path.toStdString()
            << std::endl;
}

void AIPluginManager::SaveToMetaFile(const QString& image_path)
{
  QFileInfo fileInfo(image_path);
  QString meta_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".meta";

  // Use existing ExportAnnotations logic but to .meta file
  canvas_->ExportAnnotations(meta_path, 0);
  std::cout << "Saved to meta file: " << meta_path.toStdString() << std::endl;
}

void AIPluginManager::LoadFromMetaFile(const QString& image_path)
{
  QFileInfo fileInfo(image_path);
  QString meta_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".meta";

  if (!QFile::exists(meta_path))
  {
    return;
  }

  // Get class colors
  QVector<QColor> class_colors;
  for (const auto& cls : project_config_->GetClasses())
  {
    class_colors.append(cls.color);
  }

  // Load from .meta file
  canvas_->LoadAnnotations(meta_path, class_colors);
  std::cout << "Loaded from meta file: " << meta_path.toStdString() << std::endl;
}

bool AIPluginManager::HasMetaFile(const QString& image_path) const
{
  QFileInfo fileInfo(image_path);
  QString meta_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".meta";
  return QFile::exists(meta_path);
}

bool AIPluginManager::HasApprovedFile(const QString& image_path) const
{
  QFileInfo fileInfo(image_path);
  QString label_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".txt";
  return QFile::exists(label_path);
}

void AIPluginManager::PromoteMetaToApproved(const QString& image_path)
{
  QFileInfo fileInfo(image_path);
  QString meta_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".meta";
  QString label_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".txt";

  if (!QFile::exists(meta_path))
  {
    return;
  }

  // Remove old approved file if exists
  if (QFile::exists(label_path))
  {
    QFile::remove(label_path);
  }

  // Rename .meta -> .txt (promote to approved)
  if (QFile::rename(meta_path, label_path))
  {
    std::cout << "Approved: " << fileInfo.baseName().toStdString() << std::endl;
  }
  else
  {
    std::cerr << "Failed to approve: " << fileInfo.baseName().toStdString() << std::endl;
  }
}

void AIPluginManager::DeleteMetaFile(const QString& image_path)
{
  QFileInfo fileInfo(image_path);
  QString meta_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".meta";

  if (QFile::exists(meta_path))
  {
    QFile::remove(meta_path);
    std::cout << "Rejected meta file: " << fileInfo.baseName().toStdString() << std::endl;
  }
}

int AIPluginManager::CountUnreviewedImages() const
{
  if (image_list_ == nullptr)
  {
    return 0;
  }

  int count = 0;

  for (const QString& image_file : *image_list_)
  {
    QString image_path = project_directory_ + "/images/" + image_file;

    // Unreviewed = has .meta but no .txt
    if (HasMetaFile(image_path) && !HasApprovedFile(image_path))
    {
      count++;
    }

    // Also count images with no annotations at all (not detected yet)
    if (!HasMetaFile(image_path) && !HasApprovedFile(image_path))
    {
      count++;
    }
  }

  return count;
}

void AIPluginManager::PromptModelRegistration()
{
  // Find the trained model - search recursively in runs directory
  QString trained_model_path;
  QString runs_dir = project_directory_ + "/runs";

  // Recursively search for best.pt files
  QDir runs(runs_dir);
  if (runs.exists())
  {
    QDirIterator it(runs_dir, QStringList() << "best.pt", QDir::Files,
                    QDirIterator::Subdirectories);

    QDateTime latest_time;

    while (it.hasNext())
    {
      QString file_path = it.next();
      QFileInfo file_info(file_path);

      // Find the most recently modified best.pt
      if (!latest_time.isValid() || file_info.lastModified() > latest_time)
      {
        latest_time = file_info.lastModified();
        trained_model_path = file_path;
      }
    }

    if (!trained_model_path.isEmpty())
    {
      std::cout << "Found trained model: " << trained_model_path.toStdString() << std::endl;
    }
    else
    {
      std::cout << "No best.pt found in: " << runs_dir.toStdString() << std::endl;
    }
  }

  // Copy to models/best.pt if found
  QString dest_path = project_directory_ + "/models/best.pt";
  if (!trained_model_path.isEmpty())
  {
    QDir().mkpath(project_directory_ + "/models");

    // Remove old best.pt if exists
    if (QFile::exists(dest_path))
    {
      QFile::remove(dest_path);
    }

    // Copy new model
    if (QFile::copy(trained_model_path, dest_path))
    {
      std::cout << "Copied model to: " << dest_path.toStdString() << std::endl;
    }
    else
    {
      std::cerr << "Failed to copy model to: " << dest_path.toStdString() << std::endl;
    }
  }

  QMessageBox::StandardButton reply = QMessageBox::question(
      nullptr, "Training Complete",
      QString("Training completed successfully!%1\n\nRegister this model version?")
          .arg(trained_model_path.isEmpty() ? "" : "\n\nModel saved to: models/best.pt"),
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes)
  {
    RegisterModelManually();
  }
}

void AIPluginManager::RegisterModelManually()
{
  if (image_list_ == nullptr)
  {
    return;
  }

  // Create dialog for model registration
  QDialog dialog;
  dialog.setWindowTitle("Register Model Version");
  dialog.setMinimumWidth(500);

  QVBoxLayout* layout = new QVBoxLayout(&dialog);

  // Name field
  QHBoxLayout* name_layout = new QHBoxLayout();
  name_layout->addWidget(new QLabel("Model Name:"));
  QLineEdit* name_edit = new QLineEdit();

  // Auto-generate default name
  int model_count = project_config_->GetModelVersions().size() + 1;

  // Count labeled images
  int labeled_count = 0;
  QString labels_dir = project_directory_ + "/labels";
  for (const QString& img : *image_list_)
  {
    QString label_file = labels_dir + "/" + QFileInfo(img).completeBaseName() + ".txt";
    if (QFile::exists(label_file))
    {
      labeled_count++;
    }
  }

  QString default_name = QString("model_v%1_%2imgs").arg(model_count).arg(labeled_count);
  name_edit->setText(default_name);
  name_layout->addWidget(name_edit);
  layout->addLayout(name_layout);

  // Path field
  QHBoxLayout* path_layout = new QHBoxLayout();
  path_layout->addWidget(new QLabel("Model Path:"));
  QLineEdit* path_edit = new QLineEdit();
  path_edit->setText("models/best.pt");  // Set default value
  path_edit->setPlaceholderText("models/best.pt");
  path_layout->addWidget(path_edit);

  QPushButton* browse_button = new QPushButton("Browse...");
  connect(browse_button, &QPushButton::clicked, [this, path_edit, &dialog]() {
    QString models_dir = project_directory_ + "/models";
    QString file = QFileDialog::getOpenFileName(&dialog, "Select Model File", models_dir,
                                                "Model Files (*.pt *.pth *.onnx *.h5)");
    if (!file.isEmpty())
    {
      // Make relative to project directory
      QDir project_dir(project_directory_);
      path_edit->setText(project_dir.relativeFilePath(file));
    }
  });
  path_layout->addWidget(browse_button);
  layout->addLayout(path_layout);

  // Training count (auto-filled, read-only)
  QHBoxLayout* count_layout = new QHBoxLayout();
  count_layout->addWidget(new QLabel("Training Images:"));
  QLineEdit* count_edit = new QLineEdit();
  count_edit->setText(QString::number(labeled_count));
  count_edit->setReadOnly(true);
  count_layout->addWidget(count_edit);
  layout->addLayout(count_layout);

  // Notes field
  layout->addWidget(new QLabel("Notes:"));
  QTextEdit* notes_edit = new QTextEdit();
  notes_edit->setPlaceholderText(
      "Optional notes about this model (e.g., hyperparameters, "
      "performance, purpose)");
  notes_edit->setMaximumHeight(100);
  layout->addWidget(notes_edit);

  // Buttons
  QHBoxLayout* button_layout = new QHBoxLayout();
  button_layout->addStretch();
  QPushButton* ok_button = new QPushButton("Register");
  QPushButton* cancel_button = new QPushButton("Cancel");
  button_layout->addWidget(ok_button);
  button_layout->addWidget(cancel_button);
  layout->addLayout(button_layout);

  connect(ok_button, &QPushButton::clicked, &dialog, &QDialog::accept);
  connect(cancel_button, &QPushButton::clicked, &dialog, &QDialog::reject);

  if (dialog.exec() == QDialog::Accepted)
  {
    QString name = name_edit->text().trimmed();
    QString path = path_edit->text().trimmed();
    QString notes = notes_edit->toPlainText().trimmed();

    if (name.isEmpty())
    {
      QMessageBox::warning(nullptr, "Invalid Input", "Model name cannot be empty.");
      return;
    }

    if (path.isEmpty())
    {
      path = "models/best.pt";
    }

    // Create ModelVersion
    ModelVersion model;
    model.name = name;
    model.path = path;
    model.timestamp = QDateTime::currentDateTime();
    model.training_images_count = labeled_count;
    model.notes = notes;

    std::cout << "=== Registering Model ===" << std::endl;
    std::cout << "Name: " << name.toStdString() << std::endl;
    std::cout << "Path: " << path.toStdString() << std::endl;
    std::cout << "Training images: " << labeled_count << std::endl;
    std::cout << "Notes: " << notes.toStdString() << std::endl;

    // Add to config
    project_config_->AddModelVersion(model);
    std::cout << "Total models after adding: " << project_config_->GetModelVersions().size()
              << std::endl;

    // Automatically update plugin settings to use this newly registered model for detection
    PluginConfig plugin = project_config_->GetPluginConfig();
    plugin.settings["model"] = path;  // Update detection model
    project_config_->SetPluginConfig(plugin);

    QMessageBox::information(nullptr, "Model Registered",
                             QString("Model '%1' registered successfully.\n\n"
                                     "It will be used for future detections.")
                                 .arg(name));
  }
}

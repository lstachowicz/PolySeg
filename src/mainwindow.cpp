#include "mainwindow.h"

#include <QBrush>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QShortcut>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextEdit>
#include <QVBoxLayout>

#include <iostream>

#include "aipluginmanager.h"
#include "polygoncanvas.h"
#include "settingsdialog.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      current_image_index_(-1),
      current_class_id_(-1),
      status_left_(nullptr),
      status_center_(nullptr),
      status_right_(nullptr),
      ai_plugin_manager_(nullptr)
{
  ui->setupUi(this);

  // Initialize AI Plugin Manager
  ai_plugin_manager_ = new AIPluginManager(this);
  ai_plugin_manager_->SetProjectConfig(&project_config_);
  ai_plugin_manager_->SetCanvas(ui->label);
  ai_plugin_manager_->SetStatusBar(statusBar());

  // Connect AI Plugin Manager signals
  connect(ai_plugin_manager_, &AIPluginManager::StatusMessage, this,
          [this](const QString& msg, int timeout) { statusBar()->showMessage(msg, timeout); });
  connect(ai_plugin_manager_, &AIPluginManager::ClassesUpdated, this,
          &MainWindow::UpdateStatusBar);
  connect(ai_plugin_manager_, &AIPluginManager::RequestNextUnreviewed, this,
          &MainWindow::NextUnreviewedImage);

  // Setup status bar
  status_left_ = new QLabel(this);
  status_center_ = new QLabel(this);
  status_right_ = new QLabel(this);

  statusBar()->addWidget(status_left_, 1);
  statusBar()->addWidget(status_center_, 2);
  statusBar()->addWidget(status_right_, 1);

  status_left_->setText("Ready");
  status_center_->setText("No project loaded");
  status_right_->setText("No class selected");

  // Setup recent projects menu
  recent_projects_menu_ = new QMenu("Recent Projects", this);
  ui->menuFile->insertMenu(ui->actionOpenProject, recent_projects_menu_);
  ui->menuFile->insertSeparator(ui->actionOpenProject);
  UpdateRecentProjectsMenu();

  connect(ui->actionNewProject, &QAction::triggered, this, &MainWindow::CreateNewProject);
  connect(ui->actionOpenProject, &QAction::triggered, this, &MainWindow::OpenProject);
  connect(ui->actionOpenImage, &QAction::triggered, this, &MainWindow::Load);
  connect(ui->actionAddImages, &QAction::triggered, this, &MainWindow::AddImagesToProject);
  connect(ui->actionSave, &QAction::triggered, this, &MainWindow::Save);
  connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);

  connect(ui->actionZoomIn, &QAction::triggered, this, &MainWindow::Increase);
  connect(ui->actionZoomOut, &QAction::triggered, this, &MainWindow::Decrease);
  connect(ui->actionResetZoom, &QAction::triggered, this, &MainWindow::ResetZoom);

  // Class navigation shortcuts (Ctrl+] and Ctrl+[)
  QShortcut* next_class_shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketRight), this);
  connect(next_class_shortcut, &QShortcut::activated, this, &MainWindow::NextClass);
  
  QShortcut* prev_class_shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketLeft), this);
  connect(prev_class_shortcut, &QShortcut::activated, this, &MainWindow::PreviousClass);

  // Edit menu
  connect(ui->actionUndo, &QAction::triggered, this, &MainWindow::Undo);
  connect(ui->actionRedo, &QAction::triggered, this, &MainWindow::Redo);
  connect(ui->actionCopy, &QAction::triggered, this, &MainWindow::CopyPolygon);
  connect(ui->actionPaste, &QAction::triggered, this, &MainWindow::PastePolygon);
  connect(ui->actionDelete, &QAction::triggered, [this]() { ui->label->DeleteSelectedPolygon(); });

  // Connect polygon canvas signal to auto-save
  connect(ui->label, &PolygonCanvas::PolygonsChanged, this, &MainWindow::AutoSaveCurrentImage);
  connect(ui->label, &PolygonCanvas::CurrentClassChanged, this, [this](int class_id) {
    current_class_id_ = class_id;
    UpdateStatusBar();
  });

  connect(ui->actionNextImage, &QAction::triggered, this, &MainWindow::NextImage);
  connect(ui->actionPreviousImage, &QAction::triggered, this, &MainWindow::PreviousImage);
  connect(ui->actionFirstImage, &QAction::triggered, this, &MainWindow::FirstImage);
  connect(ui->actionLastImage, &QAction::triggered, this, &MainWindow::LastImage);

  connect(ui->actionAutoDetect, &QAction::triggered, this, &MainWindow::RunAutoDetect);
  connect(ui->actionBatchDetect, &QAction::triggered, this, &MainWindow::RunBatchDetect);
  connect(ui->actionTrainModel, &QAction::triggered, this, &MainWindow::RunTrainModel);
  connect(ui->actionProjectSettings, &QAction::triggered, this, &MainWindow::ShowProjectSettings);
  connect(ui->actionProjectStatistics, &QAction::triggered, this, &MainWindow::ShowProjectStatistics);

  connect(ui->actionApproveAnnotations, &QAction::triggered, this,
          &MainWindow::ApproveCurrentAnnotations);
  connect(ui->actionRejectAnnotations, &QAction::triggered, this,
          &MainWindow::RejectCurrentAnnotations);
  connect(ui->actionNextUnreviewed, &QAction::triggered, this, &MainWindow::NextUnreviewedImage);

  // Help menu
  connect(ui->actionKeyboardShortcuts, &QAction::triggered, this,
          &MainWindow::ShowKeyboardShortcuts);
  connect(ui->actionEditShortcuts, &QAction::triggered, this, &MainWindow::EditShortcuts);
  connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::ShowAboutDialog);

  // Load keyboard shortcuts
  LoadShortcuts();
  ApplyShortcuts();

  // Load last opened project
  LoadLastProject();

  // Select first class if available
  const auto& classes = project_config_.GetClasses();
  if (!classes.isEmpty())
  {
    OnClassSelected(0);
  }
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
  // Tab/Shift+Tab for class navigation (only if not in text edit)
  if (event->key() == Qt::Key_Tab)
  {
    if (event->modifiers() & Qt::ShiftModifier)
    {
      PreviousClass();
      event->accept();
      return;
    }
    else
    {
      NextClass();
      event->accept();
      return;
    }
  }
  
  // Number keys 1-9 for quick class selection
  if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9)
  {
    int number = event->key() - Qt::Key_0;  // Convert to 1-9
    SelectClassByNumber(number);
    event->accept();
    return;
  }
  
  QMainWindow::keyPressEvent(event);
}

void MainWindow::Load()
{
  const auto home = qgetenv("HOME");
  auto filename = QFileDialog::getOpenFileName(this, tr("Open Image"), home,
                                               tr("Image Files (*.png *.jpg *.bmp)"));

  if (filename.isEmpty())
  {
    std::cout << "Active image is not selected" << std::endl;
    return;
  }

  auto pixmap = QPixmap(filename);
  ui->label->setPixmap(pixmap);
  ui->label->setFixedSize(pixmap.size());
  current_image_path_ = filename;
}

void MainWindow::Increase()
{
  ui->label->Increase();
}

void MainWindow::Decrease()
{
  ui->label->Decrease();
}

void MainWindow::ResetZoom()
{
  ui->label->ResetZoom();
}

void MainWindow::OnClassSelected(int index)
{
  const auto& classes = project_config_.GetClasses();
  if (index >= 0 && index < classes.size())
  {
    current_class_id_ = classes[index].id;
    ui->label->StartNewPolygon(current_class_id_, classes[index].color);
    UpdateStatusBar();
  }
}

void MainWindow::NextClass()
{
  const auto& classes = project_config_.GetClasses();
  int count = classes.size();
  if (count > 0)
  {
    // Find current class index
    int current_index = -1;
    for (int i = 0; i < classes.size(); ++i)
    {
      if (classes[i].id == current_class_id_)
      {
        current_index = i;
        break;
      }
    }
    
    int next = (current_index + 1) % count;
    OnClassSelected(next);
  }
}

void MainWindow::PreviousClass()
{
  const auto& classes = project_config_.GetClasses();
  int count = classes.size();
  if (count > 0)
  {
    // Find current class index
    int current_index = -1;
    for (int i = 0; i < classes.size(); ++i)
    {
      if (classes[i].id == current_class_id_)
      {
        current_index = i;
        break;
      }
    }
    
    int prev = (current_index - 1 + count) % count;
    OnClassSelected(prev);
  }
}

void MainWindow::SelectClassByNumber(int number)
{
  // Number is 1-9, convert to index 0-8
  int index = number - 1;
  const auto& classes = project_config_.GetClasses();
  if (index >= 0 && index < classes.size())
  {
    OnClassSelected(index);
  }
}


void MainWindow::AddImagesToProject()
{
  if (project_directory_.isEmpty())
  {
    QMessageBox::warning(this, "No Project",
                         "Please create or open a project first.\n\n"
                         "Use File → New Project or File → Open Project.");
    return;
  }

  QStringList files = QFileDialog::getOpenFileNames(
      this, "Select Images to Add", QDir::homePath(),
      "Images (*.jpg *.jpeg *.png *.bmp *.tiff *.tif)");

  if (files.isEmpty())
  {
    return;
  }

  // Copy images to project's images/ folder
  QString images_dir = project_directory_ + "/images";
  QDir dir;
  if (!dir.exists(images_dir))
  {
    dir.mkpath(images_dir);
  }

  int copied = 0;
  int skipped = 0;
  QStringList failed;

  for (const QString& source_path : files)
  {
    QFileInfo file_info(source_path);
    QString filename = file_info.fileName();
    
    // Generate prefix from folder structure using ImportPathConfig
    QString prefix;
    const ImportPathConfig& import_cfg = project_config_.GetImportPathConfig();
    
    // Get the directory part of the source path
    QString dir_path = file_info.absolutePath();
    
    // Remove base path if configured
    QString remaining_path = dir_path;
    if (!import_cfg.base_path.isEmpty() && dir_path.startsWith(import_cfg.base_path))
    {
      remaining_path = dir_path.mid(import_cfg.base_path.length());
      // Remove leading slash if present
      if (remaining_path.startsWith("/"))
      {
        remaining_path = remaining_path.mid(1);
      }
    }
    
    // Split path into components and filter out skip folders
    QStringList path_parts = remaining_path.split("/", Qt::SkipEmptyParts);
    QStringList filtered_parts;
    
    for (const QString& part : path_parts)
    {
      if (!import_cfg.skip_folders.contains(part))
      {
        filtered_parts.append(part);
      }
    }
    
    // Create prefix from remaining parts
    if (!filtered_parts.isEmpty())
    {
      prefix = filtered_parts.join("_") + "_";
    }
    
    // Create final filename with prefix
    QString prefixed_filename = prefix + filename;
    QString dest_path = images_dir + "/" + prefixed_filename;

    // Check if file already exists
    if (QFile::exists(dest_path))
    {
      skipped++;
      continue;
    }

    // Copy file
    if (QFile::copy(source_path, dest_path))
    {
      // Apply crop if enabled
      if (project_config_.IsCropEnabled())
      {
        const CropConfig& crop = project_config_.GetCropConfig();
        QImage image(dest_path);
        
        if (!image.isNull())
        {
          int crop_width = crop.width > 0 ? crop.width : image.width() - crop.x;
          int crop_height = crop.height > 0 ? crop.height : image.height() - crop.y;
          
          // Validate crop bounds
          if (crop.x >= 0 && crop.y >= 0 && 
              crop.x + crop_width <= image.width() && 
              crop.y + crop_height <= image.height())
          {
            QImage cropped = image.copy(crop.x, crop.y, crop_width, crop_height);
            
            // Save cropped image (overwrite the copied file)
            if (!cropped.save(dest_path))
            {
              // If save fails, remove the file and mark as failed
              QFile::remove(dest_path);
              failed.append(prefixed_filename);
              continue;
            }
          }
        }
      }
      
      copied++;
    }
    else
    {
      failed.append(prefixed_filename);
    }
  }

  // Rescan project images (this will update splits for labeled images)
  ScanProjectImages();
  SaveProjectConfig();

  // Show results
  QString message = QString("Images added: %1\n").arg(copied);
  if (skipped > 0)
  {
    message += QString("Skipped (already exist): %1\n").arg(skipped);
  }
  if (!failed.isEmpty())
  {
    message += QString("\nFailed to copy:\n%1").arg(failed.join("\n"));
  }

  QMessageBox::information(this, "Add Images Complete", message);

  // Load first new image if no image is currently loaded
  if (current_image_path_.isEmpty() && !image_list_.isEmpty())
  {
    LoadImageAtIndex(0);
  }
  else
  {
    UpdateWindowTitle();
    UpdateStatusBar();
  }
}

void MainWindow::Save()
{
  if (current_image_path_.isEmpty())
  {
    QMessageBox::warning(this, "No Image", "Please load an image first.");
    return;
  }

  auto polygons = ui->label->GetPolygons();
  if (polygons.isEmpty())
  {
    QMessageBox::warning(this, "No Annotations",
                         "Please create polygon annotations first.\n\n"
                         "Select a class, then click on the image to add points.\n"
                         "Press Enter to finish the polygon.");
    return;
  }

  // If project is open, auto-save to labels folder
  if (!project_directory_.isEmpty())
  {
    AutoSaveCurrentImage();
    QMessageBox::information(this, "Success", "Annotations saved to labels/");
    return;
  }

  // Otherwise, ask user for location (legacy behavior)
  QFileInfo fileInfo(current_image_path_);
  QString txt_path = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".txt";

  // Można też pozwolić użytkownikowi wybrać lokalizację
  QString filename = QFileDialog::getSaveFileName(this, tr("Save Annotations"), txt_path,
                                                  tr("Text Files (*.txt)"));

  if (filename.isEmpty())
  {
    return;
  }

  ui->label->ExportAnnotations(filename, 0);  // Export all polygons
  QMessageBox::information(this, "Success", "Annotations saved successfully!");
}

void MainWindow::SaveProjectConfig()
{
  project_config_.SaveToFile(project_file_path_);
}

void MainWindow::AutoSaveCurrentImage()
{
  if (current_image_path_.isEmpty() || project_directory_.isEmpty())
  {
    return;
  }

  auto polygons = ui->label->GetPolygons();

  // Get filename without extension
  QFileInfo fileInfo(current_image_path_);
  QString labelsDir = project_directory_ + "/labels";
  QString labelPath = labelsDir + "/" + fileInfo.completeBaseName() + ".txt";

  if (polygons.isEmpty())
  {
    // Remove label file if no polygons
    if (QFile::exists(labelPath))
    {
      QFile::remove(labelPath);
    }
  }
  else
  {
    // Ensure labels directory exists
    QDir dir;
    if (!dir.exists(labelsDir))
    {
      dir.mkpath(labelsDir);
    }
    
    // Save annotations
    ui->label->ExportAnnotations(labelPath, 0);
  }
}

void MainWindow::LoadImageAtIndex(int index)
{
  if (project_directory_.isEmpty() || image_list_.isEmpty())
  {
    QMessageBox::warning(this, "No Project", "Please create or open a project first.");
    return;
  }

  if (index < 0 || index >= image_list_.size())
  {
    std::cerr << "Invalid image index: " << index << std::endl;
    return;
  }

  // Auto-save current image before switching
  AutoSaveCurrentImage();

  // Load new image
  current_image_index_ = index;
  QString imagePath = project_directory_ + "/images/" + image_list_[index];

  QPixmap pixmap(imagePath);
  if (pixmap.isNull())
  {
    QMessageBox::critical(this, "Error", "Failed to load image:\n" + imagePath);
    return;
  }

  current_image_path_ = imagePath;
  ui->label->setPixmap(pixmap);
  // Don't reset zoom - keep current zoom level
  // ui->label->ResetZoom();

  // Load existing annotations if they exist
  QFileInfo fileInfo(imagePath);
  QString labelPath = project_directory_ + "/labels/" + fileInfo.completeBaseName() + ".txt";

  // Temporarily disconnect auto-save signal during loading
  disconnect(ui->label, &PolygonCanvas::PolygonsChanged, this, &MainWindow::AutoSaveCurrentImage);

  if (QFile::exists(labelPath))
  {
    // Get class colors from project config
    QVector<QColor> class_colors;
    for (const auto& cls : project_config_.GetClasses())
    {
      class_colors.append(cls.color);
    }

    ui->label->LoadAnnotations(labelPath, class_colors);
  }
  else
  {
    ui->label->ClearAllPolygons();
  }

  // Reconnect auto-save signal
  connect(ui->label, &PolygonCanvas::PolygonsChanged, this, &MainWindow::AutoSaveCurrentImage);

  UpdateWindowTitle();
  UpdateStatusBar();

  // Set focus to canvas for keyboard shortcuts
  ui->label->setFocus();
}

void MainWindow::NextImage()
{
  if (image_list_.isEmpty())
  {
    QMessageBox::warning(this, "No Project", "Please open a project first.");
    return;
  }

  // Auto-finish current polygon if drawing
  ui->label->FinishCurrentPolygon();

  int next_index = current_image_index_ + 1;
  if (next_index >= image_list_.size())
  {
    next_index = 0;  // Wrap to first image
  }

  LoadImageAtIndex(next_index);
}

void MainWindow::PreviousImage()
{
  if (image_list_.isEmpty())
  {
    QMessageBox::warning(this, "No Project", "Please open a project first.");
    return;
  }

  // Auto-finish current polygon if drawing
  ui->label->FinishCurrentPolygon();

  int prev_index = current_image_index_ - 1;
  if (prev_index < 0)
  {
    prev_index = image_list_.size() - 1;  // Wrap to last image
  }

  LoadImageAtIndex(prev_index);
}

void MainWindow::FirstImage()
{
  if (image_list_.isEmpty())
  {
    QMessageBox::warning(this, "No Project", "Please open a project first.");
    return;
  }

  // Auto-finish current polygon if drawing
  ui->label->FinishCurrentPolygon();

  LoadImageAtIndex(0);
}

void MainWindow::LastImage()
{
  if (image_list_.isEmpty())
  {
    QMessageBox::warning(this, "No Project", "Please open a project first.");
    return;
  }

  // Auto-finish current polygon if drawing
  ui->label->FinishCurrentPolygon();

  LoadImageAtIndex(image_list_.size() - 1);
}

void MainWindow::UpdateWindowTitle()
{
  QString title = "PolySeg";

  if (!project_config_.GetProjectName().isEmpty())
  {
    title += " - " + project_config_.GetProjectName();

    int total = project_config_.GetTotalImages();
    int labeled = project_config_.GetLabeledImages();
    if (total > 0)
    {
      title += QString(" [%1 labeled / %2 total]").arg(labeled).arg(total);
    }

    // Add split counts if enabled
    if (project_config_.IsSplitEnabled())
    {
      int train = project_config_.GetTrainCount();
      int val = project_config_.GetValCount();
      int test = project_config_.GetTestCount();
      title += QString(" [T:%1 V:%2 Te:%3]").arg(train).arg(val).arg(test);
    }
  }

  setWindowTitle(title);
}

void MainWindow::CreateNewProject()
{
  // Create custom dialog
  QDialog dialog(this);
  dialog.setWindowTitle("New Project");
  dialog.setMinimumWidth(500);
  
  QVBoxLayout* layout = new QVBoxLayout(&dialog);
  
  // Project name
  QLabel* name_label = new QLabel("Project name:");
  QLineEdit* name_edit = new QLineEdit("MyDataset");
  layout->addWidget(name_label);
  layout->addWidget(name_edit);
  
  // Location
  QLabel* location_label = new QLabel("Project location:");
  QHBoxLayout* location_layout = new QHBoxLayout();
  QLineEdit* location_edit = new QLineEdit();
  location_edit->setPlaceholderText("Select project location...");
  QPushButton* browse_btn = new QPushButton("Browse...");
  
  connect(browse_btn, &QPushButton::clicked, [&]() {
    const auto home = qgetenv("HOME");
    QString dir = QFileDialog::getExistingDirectory(this, "Select Project Location", home);
    if (!dir.isEmpty())
    {
      location_edit->setText(dir);
    }
  });
  
  location_layout->addWidget(location_edit);
  location_layout->addWidget(browse_btn);
  layout->addWidget(location_label);
  layout->addLayout(location_layout);

  // Buttons
  layout->addSpacing(10);
  QHBoxLayout* button_layout = new QHBoxLayout();
  button_layout->addStretch();
  QPushButton* create_btn = new QPushButton("Create");
  QPushButton* cancel_btn = new QPushButton("Cancel");
  
  connect(create_btn, &QPushButton::clicked, &dialog, &QDialog::accept);
  connect(cancel_btn, &QPushButton::clicked, &dialog, &QDialog::reject);
  
  button_layout->addWidget(create_btn);
  button_layout->addWidget(cancel_btn);
  layout->addLayout(button_layout);
  
  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }
  
  QString projectName = name_edit->text().trimmed();
  QString projectDir = location_edit->text().trimmed();
  
  if (projectName.isEmpty() || projectDir.isEmpty())
  {
    QMessageBox::warning(this, "Error", "Please provide project name and location.");
    return;
  }

  // Create project directory structure
  project_directory_ = projectDir + "/" + projectName;
  QDir dir;

  if (dir.exists(project_directory_))
  {
    QMessageBox::warning(this, "Error", "Project already exists!");
    return;
  }

  dir.mkpath(project_directory_);
  dir.mkpath(project_directory_ + "/images");
  dir.mkpath(project_directory_ + "/labels");
  dir.mkpath(project_directory_ + "/models");

  // Initialize project config
  project_config_.SetProjectName(projectName);
  project_config_.SetProjectDirectory(project_directory_);
  project_config_.UpdateStatistics(0, 0, 0);

  project_file_path_ = project_directory_ + "/" + projectName + ".polyseg";
  SaveProjectConfig();

  // Open new project in a new window
  MainWindow* newWindow = new MainWindow();
  newWindow->project_directory_ = project_directory_;
  newWindow->project_file_path_ = project_file_path_;
  newWindow->project_config_ = project_config_;
  
  // Add to recent projects
  newWindow->AddToRecentProjects(project_file_path_);
  
  // Select first class if available
  const auto& classes = newWindow->project_config_.GetClasses();
  if (!classes.isEmpty())
  {
    newWindow->OnClassSelected(0);
  }
  
  newWindow->UpdateWindowTitle();
  newWindow->show();

  QMessageBox::information(
      newWindow, "Success",
      "Project created at:\n" + project_directory_ + "\n\nAdd images to the 'images' folder.");
}

void MainWindow::OpenProject()
{
  const auto home = qgetenv("HOME");
  QString projectDir = QFileDialog::getExistingDirectory(this, "Open PolySeg Project", home);

  if (projectDir.isEmpty())
  {
    return;
  }

  // Get project name from directory
  QDir dir(projectDir);
  QString projectName = dir.dirName();
  QString projectFile = projectDir + "/" + projectName + ".polyseg";

  if (!QFile::exists(projectFile))
  {
    QMessageBox::warning(this, "Error",
                         "Not a valid PolySeg project!\n" + projectName + ".polyseg not found.");
    return;
  }

  // Load project in a new window
  MainWindow* newWindow = new MainWindow();
  newWindow->project_directory_ = projectDir;
  newWindow->project_file_path_ = projectFile;

  if (newWindow->project_config_.LoadFromFile(projectFile))
  {
    newWindow->project_config_.SetProjectDirectory(projectDir);
    
    // Add to recent projects
    newWindow->AddToRecentProjects(projectFile);
    
    // Select first class if available
    const auto& classes = newWindow->project_config_.GetClasses();
    if (!classes.isEmpty())
    {
      newWindow->OnClassSelected(0);
    }
    
    newWindow->ScanProjectImages();
    newWindow->UpdateWindowTitle();
    
    // Load first image if available
    if (!newWindow->image_list_.isEmpty())
    {
      newWindow->LoadImageAtIndex(0);
    }
    
    newWindow->show();

    QMessageBox::information(newWindow, "Project Opened",
                             QString("Loaded: %1\nImages found: %2")
                                 .arg(newWindow->project_config_.GetProjectName())
                                 .arg(newWindow->image_list_.size()));
  }
  else
  {
    delete newWindow;
    QMessageBox::warning(this, "Error", "Failed to load project!");
  }
}

void MainWindow::SaveProject()
{
  if (project_directory_.isEmpty())
  {
    QMessageBox::warning(this, "No Project", "Please create or open a project first.");
    return;
  }

  SaveProjectConfig();
  QMessageBox::information(this, "Success", "Project saved!");
}

void MainWindow::ScanProjectImages()
{
  if (project_directory_.isEmpty())
  {
    return;
  }

  image_list_.clear();
  QDir imagesDir(project_directory_ + "/images");
  QStringList filters;
  filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp";

  image_list_ = imagesDir.entryList(filters, QDir::Files, QDir::Name);
  project_config_.SetTotalImages(image_list_.size());

  // Count labeled images and build list
  int labeled = 0;
  QStringList labeled_images;
  QDir labelsDir(project_directory_ + "/labels");
  for (const QString& image : image_list_)
  {
    QFileInfo imageInfo(image);
    QString labelFile = imageInfo.completeBaseName() + ".txt";
    if (labelsDir.exists(labelFile))
    {
      labeled++;
      labeled_images.append(image);
    }
  }

  // Update image splits if enabled - only for labeled images
  if (project_config_.IsSplitEnabled())
  {
    project_config_.UpdateImageSplits(labeled_images);
  }

  int totalPolygons = 0;  // TODO: count from label files
  project_config_.UpdateStatistics(image_list_.size(), labeled, totalPolygons);

  std::cout << "Found " << image_list_.size() << " images, " << labeled << " labeled" << std::endl;

  // Show split statistics if enabled
  if (project_config_.IsSplitEnabled())
  {
    std::cout << "Split counts - Train: " << project_config_.GetTrainCount()
              << " Val: " << project_config_.GetValCount()
              << " Test: " << project_config_.GetTestCount() << std::endl;
  }

  // Update AI plugin manager with project info
  ai_plugin_manager_->SetProjectDirectory(project_directory_);
  ai_plugin_manager_->SetImageList(&image_list_);
}

// ============================================================================
// AI Plugin System (delegated to AIPluginManager)
// ============================================================================

void MainWindow::RunAutoDetect()
{
  ai_plugin_manager_->RunAutoDetect(current_image_path_);
}

void MainWindow::RunTrainModel()
{
  ai_plugin_manager_->RunTrainModel();
}

void MainWindow::RunBatchDetect()
{
  AutoSaveCurrentImage();
  ai_plugin_manager_->RunBatchDetect();
}

void MainWindow::PromptModelRegistration()
{
  ai_plugin_manager_->PromptModelRegistration();
}

void MainWindow::RegisterModelManually()
{
  ai_plugin_manager_->RegisterModelManually();
}

// ============================================================================
// Annotation Review
// ============================================================================

void MainWindow::ApproveCurrentAnnotations()
{
  if (current_image_path_.isEmpty())
  {
    QMessageBox::warning(this, "No Image", "Please load an image first.");
    return;
  }

  AutoSaveCurrentImage();
  ai_plugin_manager_->DeleteMetaFile(current_image_path_);
  statusBar()->showMessage("Annotations approved and saved!", 3000);
  NextUnreviewedImage();
}

void MainWindow::RejectCurrentAnnotations()
{
  if (current_image_path_.isEmpty())
  {
    QMessageBox::warning(this, "No Image", "Please load an image first.");
    return;
  }

  ai_plugin_manager_->DeleteMetaFile(current_image_path_);
  ui->label->ClearAllPolygons();
  statusBar()->showMessage("AI detections rejected. Canvas cleared.", 3000);
  NextUnreviewedImage();
}

void MainWindow::NextUnreviewedImage()
{
  if (image_list_.isEmpty())
  {
    QMessageBox::information(this, "No Images", "No images in project.");
    return;
  }

  int start_index = current_image_index_ + 1;

  for (int i = 0; i < image_list_.size(); ++i)
  {
    int idx = (start_index + i) % image_list_.size();
    QString image_path = project_directory_ + "/images/" + image_list_[idx];

    bool has_meta = ai_plugin_manager_->HasMetaFile(image_path);
    bool has_approved = ai_plugin_manager_->HasApprovedFile(image_path);

    if ((has_meta && !has_approved) || (!has_meta && !has_approved))
    {
      LoadImageAtIndex(idx);

      if (has_meta)
      {
        ai_plugin_manager_->LoadFromMetaFile(image_path);
        statusBar()->showMessage(
            QString("Reviewing AI detections - Edit and Approve/Reject (Image %1/%2)")
                .arg(idx + 1)
                .arg(image_list_.size()),
            5000);
      }
      else
      {
        statusBar()->showMessage(QString("No detections - Annotate manually (Image %1/%2)")
                                     .arg(idx + 1)
                                     .arg(image_list_.size()),
                                 5000);
      }
      return;
    }
  }

  QMessageBox::information(this, "Review Complete",
                           "All images have been reviewed!\n\n"
                           "No unreviewed detections remaining.");
}

void MainWindow::ShowProjectSettings()
{
  SettingsDialog dialog(project_config_, project_directory_, this);

  // Connect signal for model registration
  connect(&dialog, &SettingsDialog::RequestModelRegistration, this, [this, &dialog]() {
    RegisterModelManually();
    // After registration, refresh the model list in the dialog
    dialog.RefreshModelList();
  });

  if (dialog.exec() == QDialog::Accepted)
  {
    project_config_ = dialog.GetConfig();
    SaveProjectConfig();
    UpdateWindowTitle();
    UpdateStatusBar();
    
    // Auto-select class after settings dialog closes
    const auto& classes = project_config_.GetClasses();
    if (!classes.isEmpty())
    {
      // If current class is valid, keep it; otherwise select first class
      bool current_valid = false;
      for (const auto& pc : classes)
      {
        if (pc.id == current_class_id_)
        {
          current_valid = true;
          break;
        }
      }
      
      if (!current_valid)
      {
        OnClassSelected(0);  // Select first class
      }
    }
    
    QMessageBox::information(this, "Settings Saved",
                             "Project settings have been saved successfully.");
  }
}

void MainWindow::ShowProjectStatistics()
{
  QString stats = GetProjectStatistics();

  QDialog dialog(this);
  dialog.setWindowTitle("Project Statistics");
  dialog.setMinimumSize(700, 600);

  QVBoxLayout* layout = new QVBoxLayout(&dialog);

  QTextEdit* text = new QTextEdit(&dialog);
  text->setReadOnly(true);
  text->setHtml(stats);
  layout->addWidget(text);

  // Progress bar
  int total = image_list_.size();
  int labeled = 0;
  for (const QString& img : image_list_)
  {
    QString img_path = project_directory_ + "/images/" + img;
    if (ai_plugin_manager_->HasApprovedFile(img_path))
    {
      labeled++;
    }
  }

  QProgressBar* progress = new QProgressBar(&dialog);
  progress->setRange(0, total);
  progress->setValue(labeled);
  progress->setFormat(QString("%v/%m (%p%) labeled"));
  layout->addWidget(progress);

  QPushButton* close_btn = new QPushButton("Close", &dialog);
  connect(close_btn, &QPushButton::clicked, &dialog, &QDialog::accept);
  layout->addWidget(close_btn);

  dialog.exec();
}

void MainWindow::UpdateStatusBar()
{
  // Left: Current action (with split info if enabled)
  int polygon_count = ui->label->GetPolygons().size();
  QString left_text;

  if (!image_list_.isEmpty() && current_image_index_ >= 0 && project_config_.IsSplitEnabled())
  {
    QString img_name = image_list_[current_image_index_];
    QString split = project_config_.GetImageSplit(img_name);
    if (!split.isEmpty())
    {
      left_text = QString("(%1) ").arg(split);
    }
  }

  if (polygon_count > 0)
  {
    left_text += QString("%1 polygon%2").arg(polygon_count).arg(polygon_count == 1 ? "" : "s");
  }
  else
  {
    left_text += "No polygons";
  }
  status_left_->setText(left_text);

  // Center: Image info
  if (!image_list_.isEmpty() && current_image_index_ >= 0)
  {
    QString img_name = image_list_[current_image_index_];
    QString labelPath = project_directory_ + "/labels/" + QFileInfo(img_name).completeBaseName() + ".txt";
    QString annotated = QFile::exists(labelPath) ? " - Annotated" : "";
    QPixmap pixmap = ui->label->GetPixmap();
    status_center_->setText(QString("Image %1/%2%3 - %4 (%5x%6)")
                                .arg(current_image_index_ + 1)
                                .arg(image_list_.size())
                                .arg(annotated)
                                .arg(img_name)
                                .arg(pixmap.width())
                                .arg(pixmap.height()));
  }
  else
  {
    status_center_->setText("No image loaded");
  }

  // Right: Class info
  const auto& classes = project_config_.GetClasses();
  if (current_class_id_ >= 0 && current_class_id_ < classes.size())
  {
    const auto& pc = classes[current_class_id_];
    status_right_->setText(
        QString("Class: %1 [%2/%3]").arg(pc.name).arg(current_class_id_ + 1).arg(classes.size()));
  }
  else
  {
    status_right_->setText("Select 1-9 key to activate class");
  }
}

QString MainWindow::GetProjectStatistics() const
{
  QString html;
  html += "<html><body style='font-family: Arial;'>";
  html += "<h2>Project Statistics</h2>";
  html += "<hr>";

  // Image statistics
  int total_images = image_list_.size();
  int labeled_images = 0;
  int unlabeled_images = 0;
  
  // Count polygons per class
  QMap<int, int> class_polygon_counts;
  QMap<int, QSet<QString>> class_image_counts;  // Track which images use each class
  const auto& classes = project_config_.GetClasses();
  
  for (const auto& pc : classes)
  {
    class_polygon_counts[pc.id] = 0;
  }

  for (const QString& img : image_list_)
  {
    QString label_path = project_directory_ + "/labels/" + img;
    label_path.replace(".bmp", ".txt").replace(".jpg", ".txt").replace(".png", ".txt")
              .replace(".jpeg", ".txt").replace(".tiff", ".txt").replace(".tif", ".txt");

    QFile file(label_path);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QTextStream in(&file);
      bool has_annotations = false;
      
      while (!in.atEnd())
      {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty())
        {
          has_annotations = true;
          QStringList parts = line.split(' ', Qt::SkipEmptyParts);
          if (!parts.isEmpty())
          {
            int class_id = parts[0].toInt();
            class_polygon_counts[class_id]++;
            class_image_counts[class_id].insert(img);
          }
        }
      }
      file.close();
      
      if (has_annotations)
      {
        labeled_images++;
      }
      else
      {
        unlabeled_images++;
      }
    }
    else
    {
      unlabeled_images++;
    }
  }

  html += "<h3>📊 Images</h3>";
  html += "<table border='0' cellpadding='4'>";
  html += QString("<tr><td><b>Total images:</b></td><td>%1</td></tr>").arg(total_images);
  html += QString("<tr><td><b>Labeled images:</b></td><td>%1 (%2%)</td></tr>")
              .arg(labeled_images)
              .arg(total_images > 0 ? QString::number(labeled_images * 100.0 / total_images, 'f', 1)
                                    : "0.0");
  html += QString("<tr><td><b>Unlabeled images:</b></td><td>%1 (%2%)</td></tr>")
              .arg(unlabeled_images)
              .arg(total_images > 0 ? QString::number(unlabeled_images * 100.0 / total_images, 'f', 1)
                                    : "0.0");
  html += "</table>";

  // Dataset splits
  if (project_config_.IsSplitEnabled())
  {
    const SplitConfig& split_cfg = project_config_.GetSplitConfig();
    int train_count = 0;
    int val_count = 0;
    int test_count = 0;
    
    for (const QString& img : image_list_)
    {
      QString split = project_config_.GetImageSplit(img);
      if (split == "train") train_count++;
      else if (split == "val") val_count++;
      else if (split == "test") test_count++;
    }
    
    html += "<h3>🎯 Dataset Split</h3>";
    html += "<table border='0' cellpadding='4'>";
    html += QString("<tr><td><b>Train:</b></td><td>%1 images (%2% - target: %3%)</td></tr>")
                .arg(train_count)
                .arg(total_images > 0 ? QString::number(train_count * 100.0 / total_images, 'f', 1) : "0.0")
                .arg(QString::number(split_cfg.target_train_ratio * 100, 'f', 0));
    html += QString("<tr><td><b>Validation:</b></td><td>%1 images (%2% - target: %3%)</td></tr>")
                .arg(val_count)
                .arg(total_images > 0 ? QString::number(val_count * 100.0 / total_images, 'f', 1) : "0.0")
                .arg(QString::number(split_cfg.target_val_ratio * 100, 'f', 0));
    html += QString("<tr><td><b>Test:</b></td><td>%1 images (%2% - target: %3%)</td></tr>")
                .arg(test_count)
                .arg(total_images > 0 ? QString::number(test_count * 100.0 / total_images, 'f', 1) : "0.0")
                .arg(QString::number(split_cfg.target_test_ratio * 100, 'f', 0));
    html += "</table>";
  }
  else
  {
    html += "<h3>🎯 Dataset Split</h3>";
    html += "<p><i>Dataset splitting is disabled. Enable it in Project Settings → Dataset Splits.</i></p>";
  }

  // Class statistics
  html += "<h3>🏷️ Classes & Annotations</h3>";
  html += "<table border='1' cellpadding='6' cellspacing='0' style='border-collapse: collapse;'>";
  html += "<tr style='background-color: #f0f0f0;'>"
          "<th>Index</th><th>Class Name</th><th>Color</th><th>Polygons</th><th>Images Used</th></tr>";
  
  int total_polygons = 0;
  for (const auto& pc : classes)
  {
    int polygon_count = class_polygon_counts.value(pc.id, 0);
    int image_count = class_image_counts.value(pc.id).size();
    total_polygons += polygon_count;
    
    QString color_box = QString("<div style='width:20px;height:20px;background-color:%1;border:1px solid #000;display:inline-block;'></div>")
                            .arg(pc.color.name());
    
    html += QString("<tr><td align='center'>%1</td><td>%2</td><td align='center'>%3</td><td align='center'>%4</td><td align='center'>%5</td></tr>")
                .arg(pc.index + 1)
                .arg(pc.name)
                .arg(color_box)
                .arg(polygon_count)
                .arg(image_count);
  }
  
  html += QString("<tr style='background-color: #e8e8e8; font-weight: bold;'>"
                  "<td colspan='3' align='right'>Total:</td><td align='center'>%1</td><td align='center'>%2</td></tr>")
              .arg(total_polygons)
              .arg(labeled_images);
  html += "</table>";

  // Validation warnings
  html += "<h3>⚠️ Validation</h3>";
  html += "<table border='0' cellpadding='4'>";

  if (unlabeled_images > 0)
  {
    html += QString(
                "<tr><td>⚠️</td><td><font color='orange'>%1 image%2 without "
                "annotations</font></td></tr>")
                .arg(unlabeled_images)
                .arg(unlabeled_images == 1 ? "" : "s");
  }

  if (classes.isEmpty())
  {
    html += "<tr><td>⚠️</td><td><font color='red'>No classes defined</font></td></tr>";
  }

  if (unlabeled_images == 0 && !classes.isEmpty())
  {
    html += "<tr><td>✅</td><td><font color='green'>All images have annotations!</font></td></tr>";
  }

  html += "</table>";
  html += "</body></html>";

  return html;
}

// ============================================================================
// Edit Operations (Undo/Redo/Copy/Paste)
// ============================================================================

void MainWindow::Undo()
{
  ui->label->Undo();
  UpdateStatusBar();
}

void MainWindow::Redo()
{
  ui->label->Redo();
  UpdateStatusBar();
}

void MainWindow::CopyPolygon()
{
  ui->label->CopySelectedPolygon();
  UpdateStatusBar();
}

void MainWindow::PastePolygon()
{
  ui->label->PastePolygon();
  AutoSaveCurrentImage();
  UpdateStatusBar();
}


// ============================================================================
// Help Menu
// ============================================================================

void MainWindow::ShowKeyboardShortcuts()
{
  QDialog* dialog = new QDialog(this);
  dialog->setWindowTitle("Keyboard Shortcuts");
  dialog->resize(600, 500);

  QVBoxLayout* layout = new QVBoxLayout(dialog);

  QTextEdit* text = new QTextEdit();
  text->setReadOnly(true);

  QString html = R"(
<html>
<body>
<h2>Keyboard Shortcuts</h2>

<h3>File Operations</h3>
<table width='100%' cellpadding='4'>
<tr><td width='30%'><b>Ctrl+N</b></td><td>New Project</td></tr>
<tr><td><b>Ctrl+O</b></td><td>Open Project</td></tr>
<tr><td><b>Ctrl+S</b></td><td>Save</td></tr>
</table>

<h3>Drawing & Editing</h3>
<table width='100%' cellpadding='4'>
<tr><td width='30%'><b>Click</b></td><td>Add point to polygon</td></tr>
<tr><td><b>Enter</b></td><td>Finish polygon</td></tr>
<tr><td><b>Esc</b></td><td>Cancel drawing / Deselect</td></tr>
<tr><td><b>Del</b></td><td>Delete selected polygon</td></tr>
<tr><td><b>Drag point</b></td><td>Move point</td></tr>
<tr><td><b>Ctrl+Click</b></td><td>Insert/Remove point</td></tr>
</table>

<h3>Undo/Redo & Clipboard</h3>
<table width='100%' cellpadding='4'>
<tr><td width='30%'><b>Ctrl+Z</b></td><td>Undo</td></tr>
<tr><td><b>Ctrl+Y</b></td><td>Redo</td></tr>
<tr><td><b>Ctrl+C</b></td><td>Copy selected polygon</td></tr>
<tr><td><b>Ctrl+V</b></td><td>Paste polygon</td></tr>
</table>

<h3>Navigation</h3>
<table width='100%' cellpadding='4'>
<tr><td width='30%'><b>Right Arrow</b></td><td>Next image (auto-finishes polygon)</td></tr>
<tr><td><b>Left Arrow</b></td><td>Previous image (auto-finishes polygon)</td></tr>
<tr><td><b>Home</b></td><td>First image</td></tr>
<tr><td><b>End</b></td><td>Last image</td></tr>
</table>

<h3>AI Detection</h3>
<table width='100%' cellpadding='4'>
<tr><td width='30%'><b>Ctrl+D</b></td><td>Auto Detect (single image)</td></tr>
<tr><td><b>Ctrl+Shift+D</b></td><td>Batch Detect All</td></tr>
<tr><td><b>Ctrl+U</b></td><td>Next Unreviewed</td></tr>
<tr><td><b>Ctrl+Enter</b></td><td>Approve & Save</td></tr>
<tr><td><b>Ctrl+Backspace</b></td><td>Reject & Clear</td></tr>
</table>

<h3>View</h3>
<table width='100%' cellpadding='4'>
<tr><td width='30%'><b>+</b></td><td>Zoom In</td></tr>
<tr><td><b>-</b></td><td>Zoom Out</td></tr>
<tr><td><b>Ctrl+0</b></td><td>Reset Zoom</td></tr>
</table>

<h3>Classes</h3>
<table width='100%' cellpadding='4'>
<tr><td width='30%'><b>Tab</b></td><td>Next Class</td></tr>
<tr><td><b>Shift+Tab</b></td><td>Previous Class</td></tr>
<tr><td><b>1-9</b></td><td>Quick Select Class (first 9 classes)</td></tr>
</table>

</body>
</html>
)";

  text->setHtml(html);
  layout->addWidget(text);

  QPushButton* close_button = new QPushButton("Close");
  connect(close_button, &QPushButton::clicked, dialog, &QDialog::accept);
  layout->addWidget(close_button);

  dialog->exec();
}

void MainWindow::ShowAboutDialog()
{
  QMessageBox about;
  about.setWindowTitle("About PolySeg");
  about.setTextFormat(Qt::RichText);
  
  QString text = R"(
<h2>PolySeg</h2>
<p><b>AI-Powered Polygon Segmentation Tool</b></p>
<p>Version 1.0</p>
<br>
<p>A professional Qt6-based desktop application for creating polygon annotations 
with universal AI plugin support.</p>
<br>
<p><b>Key Features:</b></p>
<ul>
<li>Interactive polygon drawing with multi-class support</li>
<li>Universal AI plugin system (SMP, Detectron2, custom models)</li>
<li>Train/Val/Test split management</li>
<li>Model version tracking and comparison</li>
<li>Batch detection with quality control</li>
<li>Undo/Redo system</li>
<li>Copy/Paste polygons across images</li>
</ul>
<br>
<p><b>License:</b> MIT</p>
<p><b>Author:</b> Lukasz Stachowicz</p>
<p><b>Framework:</b> Qt 6.4.0 (LGPL v3)</p>
<br>
<p>Visit: <a href='https://github.com/lstachowicz/PolySeg'>github.com/lstachowicz/PolySeg</a></p>
)";

  about.setText(text);
  about.setIcon(QMessageBox::Information);
  about.exec();
}

// ============================================================================
// Shortcuts Customization
// ============================================================================

void MainWindow::EditShortcuts()
{
  SettingsDialog* dialog = new SettingsDialog(project_config_, project_directory_, this);
  dialog->SetCurrentTab(dialog->GetShortcutsTabIndex());

  connect(dialog, &SettingsDialog::ShortcutsChanged, this, [this](const QMap<QString, QString>& shortcuts) {
    shortcuts_ = shortcuts;
    ApplyShortcuts();
  });

  dialog->exec();
  delete dialog;
}

void MainWindow::LoadShortcuts()
{
  QSettings settings("PolySeg", "PolySeg");
  settings.beginGroup("Shortcuts");

  QStringList keys = settings.allKeys();
  if (keys.isEmpty())
  {
    // Use defaults - will be loaded by ShortcutsSettingsTab
    return;
  }

  for (const QString& key : keys)
  {
    shortcuts_[key] = settings.value(key).toString();
  }

  settings.endGroup();
}

void MainWindow::SaveShortcuts()
{
  QSettings settings("PolySeg", "PolySeg");
  settings.beginGroup("Shortcuts");

  settings.clear();
  for (auto it = shortcuts_.begin(); it != shortcuts_.end(); ++it)
  {
    settings.setValue(it.key(), it.value());
  }

  settings.endGroup();
  settings.sync();
}

void MainWindow::ApplyShortcuts()
{
  // Map action names to UI actions
  QMap<QString, QAction*> action_map;
  action_map["New Project"] = ui->actionNewProject;
  action_map["Open Project"] = ui->actionOpenProject;
  action_map["Save"] = ui->actionSave;
  action_map["Undo"] = ui->actionUndo;
  action_map["Redo"] = ui->actionRedo;
  action_map["Copy Polygon"] = ui->actionCopy;
  action_map["Paste Polygon"] = ui->actionPaste;
  action_map["Delete Selected"] = ui->actionDelete;
  action_map["Zoom In"] = ui->actionZoomIn;
  action_map["Zoom Out"] = ui->actionZoomOut;
  action_map["Reset Zoom"] = ui->actionResetZoom;
  action_map["Next Class"] = ui->actionNextClass;
  action_map["Previous Class"] = ui->actionPreviousClass;
  action_map["Next Image"] = ui->actionNextImage;
  action_map["Previous Image"] = ui->actionPreviousImage;
  action_map["First Image"] = ui->actionFirstImage;
  action_map["Last Image"] = ui->actionLastImage;
  action_map["Auto Detect"] = ui->actionAutoDetect;
  action_map["Batch Detect"] = ui->actionBatchDetect;
  action_map["Next Unreviewed"] = ui->actionNextUnreviewed;
  action_map["Approve & Save"] = ui->actionApproveAnnotations;
  action_map["Reject & Clear"] = ui->actionRejectAnnotations;
  action_map["Keyboard Shortcuts"] = ui->actionKeyboardShortcuts;

  // Apply shortcuts
  for (auto it = shortcuts_.begin(); it != shortcuts_.end(); ++it)
  {
    if (action_map.contains(it.key()))
    {
      QAction* action = action_map[it.key()];
      if (action)
      {
        action->setShortcut(QKeySequence(it.value()));
      }
    }
  }

  // Set application-wide shortcut context for navigation actions
  // This ensures arrow keys work even when QScrollArea or PolygonCanvas has focus
  ui->actionNextImage->setShortcutContext(Qt::ApplicationShortcut);
  ui->actionPreviousImage->setShortcutContext(Qt::ApplicationShortcut);
  ui->actionFirstImage->setShortcutContext(Qt::ApplicationShortcut);
  ui->actionLastImage->setShortcutContext(Qt::ApplicationShortcut);
}

void MainWindow::AddToRecentProjects(const QString& projectPath)
{
  QSettings settings("PolySeg", "PolySeg");
  QStringList recentProjects = settings.value("recentProjects").toStringList();
  
  // Remove if already exists
  recentProjects.removeAll(projectPath);
  
  // Add to front
  recentProjects.prepend(projectPath);
  
  // Keep only 10 most recent
  while (recentProjects.size() > 10)
  {
    recentProjects.removeLast();
  }
  
  settings.setValue("recentProjects", recentProjects);
  settings.setValue("lastProject", projectPath);
  
  UpdateRecentProjectsMenu();
}

void MainWindow::UpdateRecentProjectsMenu()
{
  recent_projects_menu_->clear();
  
  QSettings settings("PolySeg", "PolySeg");
  QStringList recentProjects = settings.value("recentProjects").toStringList();
  
  if (recentProjects.isEmpty())
  {
    QAction* emptyAction = recent_projects_menu_->addAction("No recent projects");
    emptyAction->setEnabled(false);
    return;
  }
  
  for (int i = 0; i < recentProjects.size(); ++i)
  {
    const QString& projectPath = recentProjects[i];
    
    // Check if file still exists
    if (!QFile::exists(projectPath))
    {
      continue;
    }
    
    QFileInfo fileInfo(projectPath);
    QString projectName = fileInfo.dir().dirName();
    
    QAction* action = recent_projects_menu_->addAction(QString("%1. %2").arg(i + 1).arg(projectName));
    action->setData(projectPath);
    
    connect(action, &QAction::triggered, this, &MainWindow::OpenRecentProject);
  }
  
  if (recent_projects_menu_->actions().isEmpty())
  {
    QAction* emptyAction = recent_projects_menu_->addAction("No recent projects");
    emptyAction->setEnabled(false);
  }
}

void MainWindow::OpenRecentProject()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action)
  {
    return;
  }
  
  QString projectFile = action->data().toString();
  
  if (!QFile::exists(projectFile))
  {
    QMessageBox::warning(this, "Error", "Project file not found:\n" + projectFile);
    return;
  }
  
  QFileInfo fileInfo(projectFile);
  QString projectDir = fileInfo.dir().path();
  
  // Load project in a new window
  MainWindow* newWindow = new MainWindow();
  newWindow->project_directory_ = projectDir;
  newWindow->project_file_path_ = projectFile;

  if (newWindow->project_config_.LoadFromFile(projectFile))
  {
    newWindow->project_config_.SetProjectDirectory(projectDir);
    
    // Add to recent projects
    newWindow->AddToRecentProjects(projectFile);
    
    // Select first class if available
    const auto& classes = newWindow->project_config_.GetClasses();
    if (!classes.isEmpty())
    {
      newWindow->OnClassSelected(0);
    }
    
    newWindow->ScanProjectImages();
    newWindow->UpdateWindowTitle();
    
    // Load first image if available
    if (!newWindow->image_list_.isEmpty())
    {
      newWindow->LoadImageAtIndex(0);
    }
    
    newWindow->show();
  }
  else
  {
    delete newWindow;
    QMessageBox::warning(this, "Error", "Failed to load project!");
  }
}

void MainWindow::LoadLastProject()
{
  QSettings settings("PolySeg", "PolySeg");
  QString lastProject = settings.value("lastProject").toString();
  
  if (lastProject.isEmpty() || !QFile::exists(lastProject))
  {
    std::cout << "No last project to load" << std::endl;
    return;
  }
  
  std::cout << "Loading last project: " << lastProject.toStdString() << std::endl;
  
  QFileInfo fileInfo(lastProject);
  QString projectDir = fileInfo.dir().path();
  
  project_directory_ = projectDir;
  project_file_path_ = lastProject;

  if (project_config_.LoadFromFile(lastProject))
  {
    project_config_.SetProjectDirectory(projectDir);
    
    // Select first class if available
    const auto& classes = project_config_.GetClasses();
    if (!classes.isEmpty())
    {
      OnClassSelected(0);
    }
    
    ScanProjectImages();
    UpdateWindowTitle();
    
    // Load first image if available
    if (!image_list_.isEmpty())
    {
      LoadImageAtIndex(0);
    }
    
    std::cout << "Loaded last project: " << project_config_.GetProjectName().toStdString() 
              << " with " << image_list_.size() << " images" << std::endl;
  }
  else
  {
    std::cout << "Failed to load last project" << std::endl;
  }
}



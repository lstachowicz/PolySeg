#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "projectsettingsdialog.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QStandardPaths>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QBrush>
#include <QPixmap>
#include <QIcon>
#include <QRegularExpression>
#include <QDir>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QTextEdit>
#include <QProgressBar>

#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , current_image_index_(-1)
    , current_class_id_(-1)
    , status_left_(nullptr)
    , status_center_(nullptr)
    , status_right_(nullptr)
{
    ui->setupUi(this);
    
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

    connect(ui->actionNewProject, &QAction::triggered, this, &MainWindow::CreateNewProject);
    connect(ui->actionOpenProject, &QAction::triggered, this, &MainWindow::OpenProject);
    connect(ui->actionOpenImage, &QAction::triggered, this, &MainWindow::Load);
    connect(ui->actionOpenFolder, &QAction::triggered, this, &MainWindow::OpenFolder);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::Save);
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    
    connect(ui->actionZoomIn, &QAction::triggered, this, &MainWindow::Increase);
    connect(ui->actionZoomOut, &QAction::triggered, this, &MainWindow::Decrease);
    connect(ui->actionResetZoom, &QAction::triggered, this, &MainWindow::ResetZoom);
    connect(ui->actionManageClasses, &QAction::triggered, this, &MainWindow::ManageClasses);
    
    connect(ui->actionNextImage, &QAction::triggered, this, &MainWindow::NextImage);
    connect(ui->actionPreviousImage, &QAction::triggered, this, &MainWindow::PreviousImage);
    connect(ui->actionFirstImage, &QAction::triggered, this, &MainWindow::FirstImage);
    connect(ui->actionLastImage, &QAction::triggered, this, &MainWindow::LastImage);
    
    connect(ui->actionAutoDetect, &QAction::triggered, this, &MainWindow::RunAutoDetect);
    connect(ui->actionBatchDetect, &QAction::triggered, this, &MainWindow::RunBatchDetect);
    connect(ui->actionTrainModel, &QAction::triggered, this, &MainWindow::RunTrainModel);
    connect(ui->actionConfigurePlugin, &QAction::triggered, this, &MainWindow::ConfigurePlugin);
    
    connect(ui->actionApproveAnnotations, &QAction::triggered, this, &MainWindow::ApproveCurrentAnnotations);
    connect(ui->actionRejectAnnotations, &QAction::triggered, this, &MainWindow::RejectCurrentAnnotations);
    connect(ui->actionNextUnreviewed, &QAction::triggered, this, &MainWindow::NextUnreviewedImage);
    
    connect(ui->comboBox_classes, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::OnClassSelected);
    
    // Load or create default project config
    QString docs_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    project_file_path_ = docs_path + "/default.polyseg";
    
    if (!project_config_.LoadFromFile(project_file_path_))
    {
        std::cout << "Creating new project config" << std::endl;
        SaveProjectConfig();
    }
    
    UpdateClassComboBox();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::Load()
{
    const auto home = qgetenv("HOME");
    auto filename = QFileDialog::getOpenFileName(this,
                                                 tr("Open Image"), home, tr("Image Files (*.png *.jpg *.bmp)"));

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

void MainWindow::ManageClasses()
{
    // Create modal dialog for class management
    QDialog dialog(this);
    dialog.setWindowTitle("Manage Classes");
    dialog.setMinimumWidth(400);
    dialog.setMinimumHeight(500);
    
    // Add subtle background color to distinguish from main window
    dialog.setStyleSheet("QDialog { background-color: #f5f5f5; }");
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // Info label
    QLabel* info = new QLabel("Drag classes to reorder. Index = position in list.", &dialog);
    info->setWordWrap(true);
    layout->addWidget(info);
    
    // Class list
    QListWidget* classList = new QListWidget(&dialog);
    classList->setDragDropMode(QAbstractItemView::InternalMove);
    
    // Helper lambda to refresh list
    auto refreshList = [&]() {
        classList->clear();
        const auto& classes = project_config_.GetClasses();
        for (const auto& pc : classes)
        {
            QString item_text = QString("[%1] %2").arg(pc.index).arg(pc.name);
            classList->addItem(item_text);
            int row = classList->count() - 1;
            auto item = classList->item(row);
            item->setBackground(QBrush(pc.color));
        }
        UpdateClassComboBox(); // Update main window combo box
    };
    
    refreshList();
    layout->addWidget(classList);
    
    // Buttons layout
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnAdd = new QPushButton("Add Class", &dialog);
    QPushButton* btnEdit = new QPushButton("Edit Class", &dialog);
    QPushButton* btnRemove = new QPushButton("Remove Class", &dialog);
    QPushButton* btnUp = new QPushButton("↑", &dialog);
    QPushButton* btnDown = new QPushButton("↓", &dialog);
    
    btnUp->setMaximumWidth(40);
    btnDown->setMaximumWidth(40);
    
    btnLayout->addWidget(btnUp);
    btnLayout->addWidget(btnDown);
    btnLayout->addStretch();
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnEdit);
    btnLayout->addWidget(btnRemove);
    
    layout->addLayout(btnLayout);
    
    // Close button
    QHBoxLayout* closeLayout = new QHBoxLayout();
    closeLayout->addStretch();
    QPushButton* btnClose = new QPushButton("Close", &dialog);
    closeLayout->addWidget(btnClose);
    layout->addLayout(closeLayout);
    
    // Add Class
    connect(btnAdd, &QPushButton::clicked, [&]() {
        bool ok;
        QString name = QInputDialog::getText(&dialog, "Add Class", 
                                             "Class name:", QLineEdit::Normal, "", &ok);
        if (!ok || name.isEmpty()) return;
        
        QColor color = QColorDialog::getColor(Qt::red, &dialog, "Select Class Color");
        if (!color.isValid()) return;
        
        project_config_.AddClass(name, color);
        SaveProjectConfig();
        refreshList();
    });
    
    // Edit Class
    connect(btnEdit, &QPushButton::clicked, [&]() {
        int row = classList->currentRow();
        if (row < 0) {
            QMessageBox::information(&dialog, "Edit Class", "Please select a class first.");
            return;
        }
        
        const auto& classes = project_config_.GetClasses();
        int class_id = classes[row].id;
        ProjectClass* pc = project_config_.GetClass(class_id);
        if (!pc) return;
        
        bool ok;
        QString name = QInputDialog::getText(&dialog, "Edit Class", 
                                             "Class name:", QLineEdit::Normal, pc->name, &ok);
        if (!ok || name.isEmpty()) return;
        
        QColor color = QColorDialog::getColor(pc->color, &dialog, "Select Class Color");
        if (!color.isValid()) return;
        
        project_config_.UpdateClass(class_id, name, color);
        SaveProjectConfig();
        refreshList();
        classList->setCurrentRow(row);
    });
    
    // Remove Class
    connect(btnRemove, &QPushButton::clicked, [&]() {
        int row = classList->currentRow();
        if (row < 0) {
            QMessageBox::information(&dialog, "Remove Class", "Please select a class first.");
            return;
        }
        
        const auto& classes = project_config_.GetClasses();
        int class_id = classes[row].id;
        ProjectClass* pc = project_config_.GetClass(class_id);
        if (!pc) return;
        
        auto reply = QMessageBox::question(&dialog, "Remove Class",
                                          "Remove class '" + pc->name + "'?",
                                          QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            project_config_.RemoveClass(class_id);
            project_config_.ReindexClasses();
            SaveProjectConfig();
            refreshList();
        }
    });
    
    // Move Up
    connect(btnUp, &QPushButton::clicked, [&]() {
        int row = classList->currentRow();
        if (row <= 0) return;
        
        const auto& classes = project_config_.GetClasses();
        project_config_.MoveClass(classes[row].id, -1);
        SaveProjectConfig();
        refreshList();
        classList->setCurrentRow(row - 1);
    });
    
    // Move Down
    connect(btnDown, &QPushButton::clicked, [&]() {
        int row = classList->currentRow();
        const auto& classes = project_config_.GetClasses();
        if (row < 0 || row >= classes.size() - 1) return;
        
        project_config_.MoveClass(classes[row].id, 1);
        SaveProjectConfig();
        refreshList();
        classList->setCurrentRow(row + 1);
    });
    
    // Handle drag-and-drop reordering
    connect(classList->model(), &QAbstractItemModel::rowsMoved, [&]() {
        QVector<ProjectClass> new_order;
        for (int i = 0; i < classList->count(); ++i)
        {
            QString item_text = classList->item(i)->text();
            int bracket_pos = item_text.indexOf("] ");
            if (bracket_pos >= 0)
            {
                QString name = item_text.mid(bracket_pos + 2);
                for (const auto& pc : project_config_.GetClasses())
                {
                    if (pc.name == name)
                    {
                        ProjectClass copy = pc;
                        copy.index = i;
                        new_order.append(copy);
                        break;
                    }
                }
            }
        }
        
        if (new_order.size() == project_config_.GetClasses().size())
        {
            project_config_.ReorderClasses(new_order);
            SaveProjectConfig();
            UpdateClassComboBox();
        }
    });
    
    connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::accept);
    
    dialog.exec();
}

void MainWindow::OnClassSelected(int index)
{
    const auto& classes = project_config_.GetClasses();
    if (index >= 0 && index < classes.size())
    {
        current_class_id_ = classes[index].id;
        ui->label->StartNewPolygon(current_class_id_, classes[index].color);
        std::cout << "Selected class: " << classes[index].name.toStdString() 
                  << " (id=" << current_class_id_ << ")" << std::endl;
    }
}

void MainWindow::UpdateClassComboBox()
{
    ui->comboBox_classes->clear();
    
    const auto& classes = project_config_.GetClasses();
    for (const auto& pc : classes)
    {
        // Create color icon
        QPixmap pixmap(16, 16);
        pixmap.fill(pc.color);
        QIcon icon(pixmap);
        
        QString item_text = QString("[%1] %2").arg(pc.index).arg(pc.name);
        ui->comboBox_classes->addItem(icon, item_text);
    }
    
    // Set first class as selected if available
    if (!classes.isEmpty() && ui->comboBox_classes->currentIndex() >= 0)
    {
        OnClassSelected(ui->comboBox_classes->currentIndex());
    }
}

void MainWindow::OpenFolder()
{
    const auto home = qgetenv("HOME");
    QString folder = QFileDialog::getExistingDirectory(this, "Open Image Folder", home);
    
    if (folder.isEmpty())
    {
        return;
    }
    
    QMessageBox::information(this, "Open Folder", 
                            "Folder selection: " + folder + "\nMulti-image support coming soon!");
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
    QString filename = QFileDialog::getSaveFileName(this,
                                                     tr("Save Annotations"),
                                                     txt_path,
                                                     tr("Text Files (*.txt)"));

    if (filename.isEmpty())
    {
        return;
    }

    ui->label->ExportYolo(filename, 0);  // Export all polygons
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
    QString labelPath = project_directory_ + "/labels/" + fileInfo.baseName() + ".txt";
    
    if (polygons.isEmpty())
    {
        // Remove label file if no polygons
        QFile::remove(labelPath);
        std::cout << "Removed empty label file: " << labelPath.toStdString() << std::endl;
    }
    else
    {
        // Save annotations
        ui->label->ExportYolo(labelPath, 0);
        std::cout << "Auto-saved " << polygons.size() << " polygons" << std::endl;
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
    ui->label->ResetZoom();
    
    // Load existing annotations if they exist
    QFileInfo fileInfo(imagePath);
    QString labelPath = project_directory_ + "/labels/" + fileInfo.baseName() + ".txt";
    
    if (QFile::exists(labelPath))
    {
        // Get class colors from project config
        QVector<QColor> class_colors;
        for (const auto& cls : project_config_.GetClasses())
        {
            class_colors.append(cls.color);
        }
        
        ui->label->LoadYoloAnnotations(labelPath, class_colors);
        statusBar()->showMessage(QString("Loaded image %1/%2 with annotations")
                                .arg(index + 1).arg(image_list_.size()), 3000);
    }
    else
    {
        ui->label->ClearAllPolygons();
        statusBar()->showMessage(QString("Loaded image %1/%2 (no annotations)")
                                .arg(index + 1).arg(image_list_.size()), 3000);
    }
    
    UpdateWindowTitle();
    UpdateStatusBar();
}

void MainWindow::NextImage()
{
    if (image_list_.isEmpty())
    {
        QMessageBox::warning(this, "No Project", "Please open a project first.");
        return;
    }
    
    int next_index = current_image_index_ + 1;
    if (next_index >= image_list_.size())
    {
        next_index = 0; // Wrap to first image
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
    
    int prev_index = current_image_index_ - 1;
    if (prev_index < 0)
    {
        prev_index = image_list_.size() - 1; // Wrap to last image
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
    
    LoadImageAtIndex(0);
}

void MainWindow::LastImage()
{
    if (image_list_.isEmpty())
    {
        QMessageBox::warning(this, "No Project", "Please open a project first.");
        return;
    }
    
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
            title += QString(" (%1/%2 labeled)").arg(labeled).arg(total);
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
    bool ok;
    QString projectName = QInputDialog::getText(this, "New Project", 
                                                "Project name:", QLineEdit::Normal, 
                                                "MyDataset", &ok);
    if (!ok || projectName.isEmpty())
    {
        return;
    }
    
    const auto home = qgetenv("HOME");
    QString projectDir = QFileDialog::getExistingDirectory(
        this, "Select Project Location", home);
    
    if (projectDir.isEmpty())
    {
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
    newWindow->UpdateClassComboBox();
    newWindow->UpdateWindowTitle();
    newWindow->show();
    
    QMessageBox::information(newWindow, "Success", 
                            "Project created at:\n" + project_directory_ + 
                            "\n\nAdd images to the 'images' folder.");
}

void MainWindow::OpenProject()
{
    const auto home = qgetenv("HOME");
    QString projectDir = QFileDialog::getExistingDirectory(
        this, "Open PolySeg Project", home);
    
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
                            "Not a valid PolySeg project!\n" +
                            projectName + ".polyseg not found.");
        return;
    }
    
    // Load project in a new window
    MainWindow* newWindow = new MainWindow();
    newWindow->project_directory_ = projectDir;
    newWindow->project_file_path_ = projectFile;
    
    if (newWindow->project_config_.LoadFromFile(projectFile))
    {
        newWindow->project_config_.SetProjectDirectory(projectDir);
        newWindow->UpdateClassComboBox();
        newWindow->ScanProjectImages();
        newWindow->UpdateWindowTitle();
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
    
    // Update image splits if enabled
    if (project_config_.IsSplitEnabled())
    {
        project_config_.UpdateImageSplits(image_list_);
    }
    
    // Count labeled images
    int labeled = 0;
    QDir labelsDir(project_directory_ + "/labels");
    for (const QString& image : image_list_)
    {
        QString labelFile = image;
        labelFile.replace(QRegularExpression("\\.(jpg|jpeg|png|bmp)$"), ".txt");
        if (labelsDir.exists(labelFile))
        {
            labeled++;
        }
    }
    
    int totalPolygons = 0; // TODO: count from label files
    project_config_.UpdateStatistics(image_list_.size(), labeled, totalPolygons);
    
    std::cout << "Found " << image_list_.size() << " images, " 
              << labeled << " labeled" << std::endl;
    
    // Show split statistics if enabled
    if (project_config_.IsSplitEnabled())
    {
        std::cout << "Split counts - Train: " << project_config_.GetTrainCount()
                  << " Val: " << project_config_.GetValCount()
                  << " Test: " << project_config_.GetTestCount() << std::endl;
    }
}

// ============================================================================
// AI Plugin System
// ============================================================================

bool MainWindow::IsPluginAvailable() const
{
    const PluginConfig& plugin = project_config_.GetPluginConfig();
    
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

QString MainWindow::BuildPluginCommand(const QString& args_template, 
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

void MainWindow::ExecutePluginCommand(const QString& command, const QStringList& args)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    std::cout << "Executing: " << command.toStdString();
    for (const QString& arg : args)
    {
        std::cout << " " << arg.toStdString();
    }
    std::cout << std::endl;
    
    process.start(command, args);
    
    if (!process.waitForStarted())
    {
        QMessageBox::critical(this, "Plugin Error", 
                            "Failed to start plugin command:\n" + command);
        return;
    }
    
    // Wait for process to finish (with timeout)
    if (!process.waitForFinished(30000)) // 30 second timeout
    {
        QMessageBox::warning(this, "Plugin Timeout", 
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
        QMessageBox::critical(this, "Plugin Error", 
                            QString("Plugin exited with error code %1:\n\n%2")
                            .arg(exitCode).arg(output));
        return;
    }
    
    // Parse JSON output for detection results
    ParseDetectionResults(output);
}

void MainWindow::ParseDetectionResults(const QString& json_output)
{
    QJsonDocument doc = QJsonDocument::fromJson(json_output.toUtf8());
    
    if (doc.isNull() || !doc.isObject())
    {
        QMessageBox::warning(this, "Parse Error", 
                           "Plugin did not return valid JSON output.");
        return;
    }
    
    QJsonObject root = doc.object();
    
    if (!root.contains("detections") || !root["detections"].isArray())
    {
        QMessageBox::warning(this, "Parse Error", 
                           "Plugin output missing 'detections' array.");
        return;
    }
    
    QJsonArray detections = root["detections"].toArray();
    int added = 0;
    
    // Get image dimensions for coordinate conversion
    QPixmap pixmap = ui->label->GetPixmap();
    if (pixmap.isNull())
    {
        QMessageBox::warning(this, "No Image", "No image loaded.");
        return;
    }
    
    int img_width = pixmap.width();
    int img_height = pixmap.height();
    
    for (const QJsonValue& det_val : detections)
    {
        QJsonObject det = det_val.toObject();
        
        QString class_name = det["class"].toString();
        // double confidence = det["confidence"].toDouble(0.0);  // Unused for now
        QJsonArray points_array = det["points"].toArray();
        
        if (points_array.isEmpty())
        {
            continue;
        }
        
        // Find or create class
        int class_id = -1;
        QColor class_color = Qt::red;
        for (const auto& cls : project_config_.GetClasses())
        {
            if (cls.name == class_name)
            {
                class_id = cls.id;
                class_color = cls.color;
                break;
            }
        }
        
        if (class_id == -1)
        {
            // Auto-create class with random color
            QColor color = QColor::fromHsv((project_config_.GetClasses().size() * 137) % 360, 200, 200);
            project_config_.AddClass(class_name, color);
            class_id = project_config_.GetClasses().last().id;
            class_color = color;
            UpdateClassComboBox();
            std::cout << "Auto-created class: " << class_name.toStdString() << std::endl;
        }
        
        // Convert normalized coordinates to pixel coordinates
        QVector<QPoint> polygon_points;
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
        
        if (polygon_points.size() >= 3)
        {
            ui->label->AddPolygonFromPlugin(polygon_points, class_id, class_color);
            added++;
        }
    }
    
    QMessageBox::information(this, "Detection Complete", 
                           QString("Added %1 detections from plugin.\n\nReview and adjust as needed.")
                           .arg(added));
    
    statusBar()->showMessage(QString("Plugin detected %1 objects").arg(added), 5000);
}

void MainWindow::ConfigurePlugin()
{
    // Create plugin configuration dialog
    QDialog dialog(this);
    dialog.setWindowTitle("Configure AI Plugin");
    dialog.setMinimumWidth(600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    
    // Plugin enabled checkbox
    QCheckBox* enabledCheck = new QCheckBox("Enable AI Plugin");
    enabledCheck->setChecked(project_config_.GetPluginConfig().enabled);
    mainLayout->addWidget(enabledCheck);
    
    // Plugin info group
    QGroupBox* infoGroup = new QGroupBox("Plugin Information");
    QFormLayout* infoLayout = new QFormLayout(infoGroup);
    
    QLineEdit* nameEdit = new QLineEdit(project_config_.GetPluginConfig().name);
    infoLayout->addRow("Plugin Name:", nameEdit);
    
    mainLayout->addWidget(infoGroup);
    
    // Command configuration group
    QGroupBox* cmdGroup = new QGroupBox("Command Configuration");
    QFormLayout* cmdLayout = new QFormLayout(cmdGroup);
    
    QLineEdit* commandEdit = new QLineEdit(project_config_.GetPluginConfig().command);
    cmdLayout->addRow("Command:", commandEdit);
    
    QHBoxLayout* scriptLayout = new QHBoxLayout();
    QLineEdit* scriptEdit = new QLineEdit(project_config_.GetPluginConfig().script_path);
    QPushButton* browseBtn = new QPushButton("Browse...");
    connect(browseBtn, &QPushButton::clicked, [&]() {
        QString file = QFileDialog::getOpenFileName(&dialog, "Select Plugin Script", 
                                                    project_directory_,
                                                    "Python Scripts (*.py);;All Files (*)");
        if (!file.isEmpty())
        {
            // Make path relative to project if possible
            if (file.startsWith(project_directory_))
            {
                file = file.mid(project_directory_.length() + 1);
            }
            scriptEdit->setText(file);
        }
    });
    scriptLayout->addWidget(scriptEdit);
    scriptLayout->addWidget(browseBtn);
    cmdLayout->addRow("Script Path:", scriptLayout);
    
    QLineEdit* detectArgsEdit = new QLineEdit(project_config_.GetPluginConfig().detect_args);
    cmdLayout->addRow("Detect Args:", detectArgsEdit);
    
    QLineEdit* trainArgsEdit = new QLineEdit(project_config_.GetPluginConfig().train_args);
    cmdLayout->addRow("Train Args:", trainArgsEdit);
    
    mainLayout->addWidget(cmdGroup);
    
    // Settings group
    QGroupBox* settingsGroup = new QGroupBox("Plugin Settings");
    QFormLayout* settingsLayout = new QFormLayout(settingsGroup);
    
    QMap<QString, QLineEdit*> settingEdits;
    const QMap<QString, QString>& settings = project_config_.GetPluginConfig().settings;
    for (auto it = settings.begin(); it != settings.end(); ++it)
    {
        QLineEdit* edit = new QLineEdit(it.value());
        settingsLayout->addRow(it.key() + ":", edit);
        settingEdits[it.key()] = edit;
    }
    
    // Add new setting button
    QPushButton* addSettingBtn = new QPushButton("Add Setting");
    connect(addSettingBtn, &QPushButton::clicked, [&]() {
        bool ok;
        QString key = QInputDialog::getText(&dialog, "Add Setting", "Setting name:", 
                                           QLineEdit::Normal, "", &ok);
        if (ok && !key.isEmpty())
        {
            QLineEdit* edit = new QLineEdit();
            settingsLayout->addRow(key + ":", edit);
            settingEdits[key] = edit;
        }
    });
    settingsLayout->addRow("", addSettingBtn);
    
    mainLayout->addWidget(settingsGroup);
    
    // Help text
    QLabel* helpLabel = new QLabel(
        "<b>Variable Substitution:</b><br>"
        "{image} - Current image path<br>"
        "{project} - Project directory<br>"
        "{model} - Model path from settings<br>"
        "{confidence} - Confidence threshold from settings<br>"
        "Any {key} from Plugin Settings"
    );
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("background-color: #f0f0f0; padding: 10px; border-radius: 5px;");
    mainLayout->addWidget(helpLabel);
    
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    
    if (dialog.exec() == QDialog::Accepted)
    {
        // Save plugin configuration
        PluginConfig plugin;
        plugin.enabled = enabledCheck->isChecked();
        plugin.name = nameEdit->text();
        plugin.command = commandEdit->text();
        plugin.script_path = scriptEdit->text();
        plugin.detect_args = detectArgsEdit->text();
        plugin.train_args = trainArgsEdit->text();
        
        // Save all settings
        for (auto it = settingEdits.begin(); it != settingEdits.end(); ++it)
        {
            plugin.settings[it.key()] = it.value()->text();
        }
        
        project_config_.SetPluginConfig(plugin);
        SaveProjectConfig();
        
        QMessageBox::information(this, "Success", "Plugin configuration saved!");
    }
}

void MainWindow::RunAutoDetect()
{
    if (!IsPluginAvailable())
    {
        QMessageBox::warning(this, "Plugin Not Available", 
                           "AI plugin is not configured or script not found.\n\n"
                           "Go to Tools → Configure Plugin to set it up.");
        return;
    }
    
    if (current_image_path_.isEmpty())
    {
        QMessageBox::warning(this, "No Image", "Please load an image first.");
        return;
    }
    
    const PluginConfig& plugin = project_config_.GetPluginConfig();
    
    // Build variable substitutions
    QMap<QString, QString> vars;
    vars["image"] = current_image_path_;
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

void MainWindow::RunTrainModel()
{
    if (!IsPluginAvailable())
    {
        QMessageBox::warning(this, "Plugin Not Available", 
                           "AI plugin is not configured or script not found.\n\n"
                           "Go to Tools → Configure Plugin to set it up.");
        return;
    }
    
    if (project_directory_.isEmpty())
    {
        QMessageBox::warning(this, "No Project", "Please open a project first.");
        return;
    }
    
    // Generate split files if enabled
    if (project_config_.IsSplitEnabled())
    {
        project_config_.GenerateSplitFiles(project_directory_);
    }
    
    const PluginConfig& plugin = project_config_.GetPluginConfig();
    
    // Build variable substitutions
    QMap<QString, QString> vars;
    vars["project"] = project_directory_;
    vars["dataset"] = project_directory_; // Same as project for now
    vars["splits"] = project_directory_ + "/splits";
    vars["train_count"] = QString::number(project_config_.GetTrainCount());
    vars["val_count"] = QString::number(project_config_.GetValCount());
    vars["test_count"] = QString::number(project_config_.GetTestCount());
    
    // Add all plugin settings as variables
    for (auto it = plugin.settings.begin(); it != plugin.settings.end(); ++it)
    {
        vars[it.key()] = it.value();
    }
    
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
    
    QMessageBox::information(this, "Training Started", 
                           "Training will run in the background.\n\n"
                           "Check the terminal for progress.");
    
    ExecutePluginCommand(plugin.command, args);
}

// ============================================================================
// Batch Detection & Meta File Management
// ============================================================================

void MainWindow::RunBatchDetect()
{
    if (!IsPluginAvailable())
    {
        QMessageBox::warning(this, "Plugin Not Available", 
                           "AI plugin is not configured or script not found.\n\n"
                           "Go to Tools → Configure Plugin to set it up.");
        return;
    }
    
    if (project_directory_.isEmpty() || image_list_.isEmpty())
    {
        QMessageBox::warning(this, "No Project", "Please open a project with images first.");
        return;
    }
    
    // Confirm batch operation
    int total_images = image_list_.size();
    int unreviewed = CountUnreviewedImages();
    
    QString message = QString("Run AI detection on all %1 images in this project?\n\n"
                             "Results will be saved as .meta files for review.\n"
                             "You can approve/reject each detection individually.\n\n"
                             "Images already reviewed: %2\n"
                             "Images to process: %3")
                        .arg(total_images)
                        .arg(total_images - unreviewed)
                        .arg(unreviewed);
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Batch Detection", message,
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes)
    {
        return;
    }
    
    // Save current work
    AutoSaveCurrentImage();
    
    // Progress tracking
    int processed = 0;
    int detected = 0;
    int skipped = 0;
    
    statusBar()->showMessage("Batch detection in progress...");
    
    // Process each image
    for (const QString& image_file : image_list_)
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
        statusBar()->showMessage(QString("Batch detection: %1/%2 processed, %3 detected")
                                .arg(processed).arg(total_images).arg(detected));
        
        // Allow UI updates
        QCoreApplication::processEvents();
    }
    
    // Show summary
    QString summary = QString("Batch detection complete!\n\n"
                             "Processed: %1 images\n"
                             "Detections found: %2 images\n"
                             "Skipped (approved): %3 images\n\n"
                             "Use Tools → Next Unreviewed to review detections.")
                        .arg(processed).arg(detected).arg(skipped);
    
    QMessageBox::information(this, "Batch Detection Complete", summary);
    
    statusBar()->showMessage(QString("Batch detection complete: %1 detected, %2 skipped")
                            .arg(detected).arg(skipped), 10000);
    
    // Jump to first unreviewed image
    NextUnreviewedImage();
}

void MainWindow::BatchDetectOnImage(const QString& image_path)
{
    const PluginConfig& plugin = project_config_.GetPluginConfig();
    
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
    
    process.start(plugin.command, args);
    
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
        std::cerr << "Plugin error for: " << image_path.toStdString() 
                  << " (exit code: " << exitCode << ")" << std::endl;
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
    
    // Image dimensions not needed - coordinates already normalized in JSON
    
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
        for (const auto& cls : project_config_.GetClasses())
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
            QColor color = QColor::fromHsv((project_config_.GetClasses().size() * 137) % 360, 200, 200);
            project_config_.AddClass(class_name, color);
            class_id = project_config_.GetClasses().last().id;
            SaveProjectConfig();
        }
        
        // Write polygon in normalized format
        out << class_id;
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
        out << "\n";
    }
    
    meta_file.close();
    std::cout << "Saved " << detections.size() << " detections to: " 
              << meta_path.toStdString() << std::endl;
}

void MainWindow::SaveToMetaFile(const QString& image_path)
{
    QFileInfo fileInfo(image_path);
    QString meta_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".meta";
    
    // Use existing ExportYolo logic but to .meta file
    ui->label->ExportYolo(meta_path, 0);
    std::cout << "Saved to meta file: " << meta_path.toStdString() << std::endl;
}

void MainWindow::LoadFromMetaFile(const QString& image_path)
{
    QFileInfo fileInfo(image_path);
    QString meta_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".meta";
    
    if (!QFile::exists(meta_path))
    {
        return;
    }
    
    // Get class colors
    QVector<QColor> class_colors;
    for (const auto& cls : project_config_.GetClasses())
    {
        class_colors.append(cls.color);
    }
    
    // Load from .meta file
    ui->label->LoadYoloAnnotations(meta_path, class_colors);
    std::cout << "Loaded from meta file: " << meta_path.toStdString() << std::endl;
}

bool MainWindow::HasMetaFile(const QString& image_path) const
{
    QFileInfo fileInfo(image_path);
    QString meta_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".meta";
    return QFile::exists(meta_path);
}

bool MainWindow::HasApprovedFile(const QString& image_path) const
{
    QFileInfo fileInfo(image_path);
    QString label_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".txt";
    return QFile::exists(label_path);
}

void MainWindow::PromoteMetaToApproved(const QString& image_path)
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
    
    // Rename .meta → .txt (promote to approved)
    if (QFile::rename(meta_path, label_path))
    {
        std::cout << "Approved: " << fileInfo.baseName().toStdString() << std::endl;
    }
    else
    {
        std::cerr << "Failed to approve: " << fileInfo.baseName().toStdString() << std::endl;
    }
}

void MainWindow::DeleteMetaFile(const QString& image_path)
{
    QFileInfo fileInfo(image_path);
    QString meta_path = project_directory_ + "/labels/" + fileInfo.baseName() + ".meta";
    
    if (QFile::exists(meta_path))
    {
        QFile::remove(meta_path);
        std::cout << "Rejected meta file: " << fileInfo.baseName().toStdString() << std::endl;
    }
}

int MainWindow::CountUnreviewedImages() const
{
    int count = 0;
    
    for (const QString& image_file : image_list_)
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

void MainWindow::ApproveCurrentAnnotations()
{
    if (current_image_path_.isEmpty())
    {
        QMessageBox::warning(this, "No Image", "Please load an image first.");
        return;
    }
    
    // Save current edits to .txt (approved)
    AutoSaveCurrentImage();
    
    // Delete .meta if exists (no longer needed)
    DeleteMetaFile(current_image_path_);
    
    statusBar()->showMessage("Annotations approved and saved!", 3000);
    
    // Auto-jump to next unreviewed
    NextUnreviewedImage();
}

void MainWindow::RejectCurrentAnnotations()
{
    if (current_image_path_.isEmpty())
    {
        QMessageBox::warning(this, "No Image", "Please load an image first.");
        return;
    }
    
    // Delete .meta file
    DeleteMetaFile(current_image_path_);
    
    // Clear canvas
    ui->label->ClearAllPolygons();
    
    statusBar()->showMessage("AI detections rejected. Canvas cleared.", 3000);
    
    // Auto-jump to next unreviewed
    NextUnreviewedImage();
}

void MainWindow::NextUnreviewedImage()
{
    if (image_list_.isEmpty())
    {
        QMessageBox::information(this, "No Images", "No images in project.");
        return;
    }
    
    // Find next unreviewed image (has .meta OR no annotations)
    int start_index = current_image_index_ + 1;
    
    for (int i = 0; i < image_list_.size(); ++i)
    {
        int idx = (start_index + i) % image_list_.size();
        QString image_path = project_directory_ + "/images/" + image_list_[idx];
        
        bool has_meta = HasMetaFile(image_path);
        bool has_approved = HasApprovedFile(image_path);
        
        // Unreviewed = (has .meta but not approved) OR (no annotations at all)
        if ((has_meta && !has_approved) || (!has_meta && !has_approved))
        {
            LoadImageAtIndex(idx);
            
            // Load from .meta if exists
            if (has_meta)
            {
                LoadFromMetaFile(image_path);
                statusBar()->showMessage(
                    QString("Reviewing AI detections - Edit and Approve/Reject (Image %1/%2)")
                    .arg(idx + 1).arg(image_list_.size()), 
                    5000
                );
            }
            else
            {
                statusBar()->showMessage(
                    QString("No detections - Annotate manually (Image %1/%2)")
                    .arg(idx + 1).arg(image_list_.size()), 
                    5000
                );
            }
            
            return;
        }
    }
    
    // All images reviewed!
    QMessageBox::information(this, "Review Complete", 
                           "All images have been reviewed!\n\n"
                           "No unreviewed detections remaining.");
}

void MainWindow::ShowProjectSettings()
{
    ProjectSettingsDialog dialog(project_config_, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        project_config_ = dialog.GetConfig();
        SaveProjectConfig();
        UpdateWindowTitle();
        UpdateStatusBar();
        QMessageBox::information(this, "Settings Saved", 
                               "Project settings have been saved successfully.");
    }
}

void MainWindow::ShowProjectStatistics()
{
    QString stats = GetProjectStatistics();
    
    QDialog dialog(this);
    dialog.setWindowTitle("Project Statistics");
    dialog.setMinimumSize(500, 400);
    
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
        if (HasApprovedFile(img_path))
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
        QPixmap pixmap = ui->label->GetPixmap();
        status_center_->setText(QString("Image %1/%2 - %3 (%4x%5)")
                               .arg(current_image_index_ + 1)
                               .arg(image_list_.size())
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
        status_right_->setText(QString("Class: %1 [%2/%3]")
                              .arg(pc.name)
                              .arg(current_class_id_ + 1)
                              .arg(classes.size()));
    }
    else
    {
        status_right_->setText("No class selected");
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
    int pending_review = 0;
    
    for (const QString& img : image_list_)
    {
        QString img_path = project_directory_ + "/images/" + img;
        bool has_approved = HasApprovedFile(img_path);
        bool has_meta = HasMetaFile(img_path);
        
        if (has_approved)
        {
            labeled_images++;
        }
        else if (has_meta)
        {
            pending_review++;
        }
        else
        {
            unlabeled_images++;
        }
    }
    
    html += "<h3>Images</h3>";
    html += "<table border='0' cellpadding='4'>";
    html += QString("<tr><td><b>Total images:</b></td><td>%1</td></tr>").arg(total_images);
    html += QString("<tr><td><b>Labeled images:</b></td><td>%1 (%2%)</td></tr>")
           .arg(labeled_images)
           .arg(total_images > 0 ? QString::number(labeled_images * 100.0 / total_images, 'f', 1) : "0");
    html += QString("<tr><td><b>Unlabeled images:</b></td><td>%1</td></tr>").arg(unlabeled_images);
    html += QString("<tr><td><b>Pending review:</b></td><td>%1</td></tr>").arg(pending_review);
    html += "</table>";
    
    // Class statistics
    const auto& classes = project_config_.GetClasses();
    html += "<h3>Classes</h3>";
    html += "<table border='0' cellpadding='4'>";
    html += QString("<tr><td><b>Total classes:</b></td><td>%1</td></tr>").arg(classes.size());
    
    // Count polygons per class (would need to scan all label files)
    html += "</table>";
    
    // Validation warnings
    html += "<h3>Validation</h3>";
    html += "<table border='0' cellpadding='4'>";
    
    if (unlabeled_images > 0)
    {
        html += QString("<tr><td>⚠️</td><td><font color='orange'>%1 image%2 without annotations</font></td></tr>")
               .arg(unlabeled_images)
               .arg(unlabeled_images == 1 ? "" : "s");
    }
    
    if (pending_review > 0)
    {
        html += QString("<tr><td>⚠️</td><td><font color='blue'>%1 image%2 pending review</font></td></tr>")
               .arg(pending_review)
               .arg(pending_review == 1 ? "" : "s");
    }
    
    if (classes.isEmpty())
    {
        html += "<tr><td>⚠️</td><td><font color='red'>No classes defined</font></td></tr>";
    }
    
    if (unlabeled_images == 0 && pending_review == 0 && !classes.isEmpty())
    {
        html += "<tr><td>✅</td><td><font color='green'>No validation issues</font></td></tr>";
    }
    
    html += "</table>";
    html += "</body></html>";
    
    return html;
}

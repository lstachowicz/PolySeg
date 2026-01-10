#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::Load);
    connect(ui->pb_save, &QPushButton::clicked, this, &MainWindow::Save);
    connect(ui->pb_increase, &QPushButton::clicked, this, &MainWindow::Increase);
    connect(ui->pb_decrease, &QPushButton::clicked, this, &MainWindow::Decrease);
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
        QMessageBox::warning(this, "No Annotations", "Please create some annotations first.");
        return;
    }

    // Tworzenie nazwy pliku .txt na podstawie nazwy obrazu
    QFileInfo fileInfo(current_image_path_);
    QString txt_path = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".txt";

    // Można też pozwolić użytkownikowi wybrać lokalizację
    QString filename = QFileDialog::getSaveFileName(this,
                                                     tr("Save YOLO Label"),
                                                     txt_path,
                                                     tr("Text Files (*.txt)"));

    if (filename.isEmpty())
    {
        return;
    }

    ui->label->ExportYolo(filename, 0);  // Export all polygons
    QMessageBox::information(this, "Success", "YOLO label saved successfully!");
}

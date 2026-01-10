#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void Load();
    void Save();

    void Increase();
    void Decrease();

private:
    Ui::MainWindow *ui;
    QString current_image_path_;
};
#endif // MAINWINDOW_H

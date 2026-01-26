#include <QApplication>

#include "mainwindow.h"

int main(int argc, char* argv[])
{
#ifdef Q_OS_LINUX
  // Set Wayland decoration style if running on Wayland and not already set
  // This ensures window frames are displayed on Wayland compositors
  if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY") &&
      !qEnvironmentVariableIsSet("QT_WAYLAND_DECORATION"))
  {
    qputenv("QT_WAYLAND_DECORATION", "adwaita");
  }
#endif

  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}

#include <QApplication>
#include <QtGlobal>

#include "logger.h"
#include "mainwindow.h"

static void SpdlogQtMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
  const std::string s = msg.toStdString();
  switch (type)
  {
    case QtWarningMsg:
      spdlog::warn("{}", s);
      break;
    case QtCriticalMsg:
      spdlog::error("{}", s);
      break;
    case QtFatalMsg:
      spdlog::critical("{}", s);
      break;
    default:
      spdlog::info("{}", s);
      break;
  }
}

int main(int argc, char* argv[])
{
  Logger::Init();
  qInstallMessageHandler(SpdlogQtMessageHandler);

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

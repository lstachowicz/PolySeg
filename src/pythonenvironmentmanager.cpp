#include "pythonenvironmentmanager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

// PythonInfo implementation

PythonInfo::PythonInfo()
    : version_major(0),
      version_minor(0),
      has_venv(false),
      has_pip(false),
      has_cuda(false),
      is_valid(false)
{
}

QString PythonInfo::GetDisplayString() const
{
  if (!is_valid)
  {
    return "Python not found";
  }

  QString display = QString("Python %1 (%2)").arg(version, path);

  if (has_cuda)
  {
    display += QString("\nCUDA: Available (%1)").arg(cuda_version);
    if (!torch_version.isEmpty())
    {
      display += QString(" - PyTorch %1").arg(torch_version);
    }
  }
  else
  {
    display += "\nCUDA: Not available (CPU mode)";
  }

  if (!has_venv)
  {
    display += "\nWarning: venv module not available";
  }

  if (!has_pip)
  {
    display += "\nWarning: pip not available";
  }

  return display;
}

bool PythonInfo::MeetsMinimumRequirements() const
{
  return is_valid && version_major >= 3 && version_minor >= 8 && has_pip;
}

// PythonEnvironmentManager implementation

PythonEnvironmentManager::PythonEnvironmentManager(QObject* parent)
    : QObject(parent),
      detection_performed_(false),
      current_process_(nullptr),
      current_operation_(OperationType::None)
{
}

PythonEnvironmentManager::~PythonEnvironmentManager()
{
  Cancel();
}

PythonInfo PythonEnvironmentManager::DetectPython()
{
  python_info_ = PythonInfo();
  detection_performed_ = true;

  emit detectionProgress("Searching for Python...");

  QStringList python_candidates = {"python3", "python"};

#ifdef Q_OS_WIN
  python_candidates << "py"
                    << "C:/Python311/python.exe"
                    << "C:/Python310/python.exe"
                    << "C:/Python39/python.exe";
#else
  python_candidates << "/usr/bin/python3"
                    << "/usr/local/bin/python3"
                    << "/opt/homebrew/bin/python3";
#endif

  for (const QString& python : python_candidates)
  {
    QString version_output = RunPythonCommand(python, {"--version"}, 5000);
    if (!version_output.isEmpty())
    {
      python_info_.path = python;
      ParsePythonVersion(version_output);

      if (python_info_.version_major >= 3)
      {
        python_info_.is_valid = true;

        QString which_output =
            RunPythonCommand(python, {"-c", "import sys; print(sys.executable)"}, 5000);
        if (!which_output.isEmpty())
        {
          python_info_.path = which_output.trimmed();
        }

        break;
      }
    }
  }

  if (!python_info_.is_valid)
  {
    emit detectionProgress("Python not found");
    emit detectionFinished(python_info_);
    return python_info_;
  }

  emit detectionProgress("Checking venv support...");
  python_info_.has_venv = CheckModuleAvailable(python_info_.path, "venv");

  emit detectionProgress("Checking pip...");
  python_info_.has_pip = CheckModuleAvailable(python_info_.path, "pip");

  emit detectionProgress("Checking CUDA support...");
  DetectCudaSupport(python_info_.path);

  emit detectionProgress("Detection complete");
  emit detectionFinished(python_info_);

  return python_info_;
}

QString PythonEnvironmentManager::GetVenvPath(const QString& project_dir)
{
  return QDir(project_dir).filePath(".venv");
}

QString PythonEnvironmentManager::GetVenvPythonPath(const QString& venv_path)
{
#ifdef Q_OS_WIN
  return QDir(venv_path).filePath("Scripts/python.exe");
#else
  return QDir(venv_path).filePath("bin/python3");
#endif
}

QString PythonEnvironmentManager::GetVenvPipPath(const QString& venv_path)
{
#ifdef Q_OS_WIN
  return QDir(venv_path).filePath("Scripts/pip.exe");
#else
  return QDir(venv_path).filePath("bin/pip");
#endif
}

QString PythonEnvironmentManager::GetVenvActivateCommand(const QString& venv_path)
{
#ifdef Q_OS_WIN
  return QString("\"%1\\Scripts\\activate.bat\"").arg(venv_path);
#else
  return QString("source \"%1/bin/activate\"").arg(venv_path);
#endif
}

bool PythonEnvironmentManager::HasProjectVenv(const QString& project_dir)
{
  QString venv_path = GetVenvPath(project_dir);
  QString python_path = GetVenvPythonPath(venv_path);
  return QFile::exists(python_path);
}

void PythonEnvironmentManager::CreateProjectVenv(const QString& project_dir)
{
  if (!python_info_.is_valid)
  {
    emit venvCreationFinished(false, QString(), "Python not detected. Run DetectPython() first.");
    return;
  }

  if (!python_info_.has_venv)
  {
    emit venvCreationFinished(false, QString(), "Python venv module is not available.");
    return;
  }

  QString venv_path = GetVenvPath(project_dir);

  if (HasProjectVenv(project_dir))
  {
    emit venvCreationProgress("Virtual environment already exists");
    emit venvCreationFinished(true, venv_path, QString());
    return;
  }

  emit venvCreationProgress("Creating virtual environment...");
  current_operation_ = OperationType::VenvCreation;
  current_venv_path_ = venv_path;

  QProcess process;
  process.setProgram(python_info_.path);
  process.setArguments({"-m", "venv", venv_path});
  process.start();

  if (!process.waitForFinished(120000))
  {
    emit venvCreationFinished(false, QString(), "Timeout creating virtual environment");
    current_operation_ = OperationType::None;
    return;
  }

  if (process.exitCode() != 0)
  {
    QString error = process.readAllStandardError();
    emit venvCreationFinished(false, QString(), QString("Failed to create venv: %1").arg(error));
    current_operation_ = OperationType::None;
    return;
  }

  if (!HasProjectVenv(project_dir))
  {
    emit venvCreationFinished(false, QString(),
                              "Virtual environment creation failed - python not found in venv");
    current_operation_ = OperationType::None;
    return;
  }

  emit venvCreationProgress("Upgrading pip...");
  QString pip_path = GetVenvPipPath(venv_path);
  QProcess pip_upgrade;
  pip_upgrade.setProgram(pip_path);
  pip_upgrade.setArguments({"install", "--upgrade", "pip"});
  pip_upgrade.start();
  pip_upgrade.waitForFinished(60000);

  emit venvCreationProgress("Virtual environment created successfully");
  emit venvCreationFinished(true, venv_path, QString());
  current_operation_ = OperationType::None;
}

void PythonEnvironmentManager::InstallRequirements(const QString& requirements_file,
                                                   const QString& venv_path)
{
  if (!QFile::exists(requirements_file))
  {
    InstallationResult result;
    result.success = false;
    result.error_message = QString("Requirements file not found: %1").arg(requirements_file);
    emit installationFinished(result);
    return;
  }

  emit installationProgress(
      QString("Installing from %1...").arg(QFileInfo(requirements_file).fileName()), -1);

  QString pip_path = GetPipPath(venv_path);
  QStringList args = {"install", "-r", requirements_file};

  QProcess process;
  process.setProgram(pip_path);
  process.setArguments(args);
  process.start();

  InstallationResult result;

  if (!process.waitForFinished(600000))
  {
    result.success = false;
    result.error_message = "Installation timeout (10 minutes)";
    emit installationFinished(result);
    return;
  }

  result.output = process.readAllStandardOutput();
  QString stderr_output = process.readAllStandardError();

  if (process.exitCode() != 0)
  {
    result.success = false;
    result.error_message = stderr_output.isEmpty() ? "Installation failed" : stderr_output;
    emit installationFinished(result);
    return;
  }

  result.success = true;

  QRegularExpression installed_regex("Successfully installed (.+)");
  QRegularExpressionMatch match = installed_regex.match(result.output);
  if (match.hasMatch())
  {
    result.installed_packages = match.captured(1).split(' ', Qt::SkipEmptyParts);
  }

  emit installationProgress("Installation complete", 100);
  emit installationFinished(result);
}

void PythonEnvironmentManager::InstallPackage(const QString& package_name, const QString& venv_path)
{
  InstallPackages({package_name}, venv_path);
}

void PythonEnvironmentManager::InstallPackages(const QStringList& packages, const QString& venv_path)
{
  if (packages.isEmpty())
  {
    InstallationResult result;
    result.success = true;
    emit installationFinished(result);
    return;
  }

  emit installationProgress(QString("Installing %1 package(s)...").arg(packages.size()), -1);

  QString pip_path = GetPipPath(venv_path);
  QStringList args = {"install"};
  args.append(packages);

  QProcess process;
  process.setProgram(pip_path);
  process.setArguments(args);
  process.start();

  InstallationResult result;

  if (!process.waitForFinished(600000))
  {
    result.success = false;
    result.error_message = "Installation timeout";
    emit installationFinished(result);
    return;
  }

  result.output = process.readAllStandardOutput();
  QString stderr_output = process.readAllStandardError();

  if (process.exitCode() != 0)
  {
    result.success = false;
    result.error_message = stderr_output.isEmpty() ? "Installation failed" : stderr_output;

    for (const QString& pkg : packages)
    {
      if (stderr_output.contains(pkg, Qt::CaseInsensitive))
      {
        result.failed_packages.append(pkg);
      }
    }

    emit installationFinished(result);
    return;
  }

  result.success = true;
  result.installed_packages = packages;

  emit installationProgress("Installation complete", 100);
  emit installationFinished(result);
}

bool PythonEnvironmentManager::IsPackageInstalled(const QString& package_name,
                                                  const QString& venv_path)
{
  QString python_path = GetPythonPath(venv_path);
  QString output = RunPythonCommand(
      python_path, {"-c", QString("import %1; print('OK')").arg(package_name)}, 10000);
  return output.trimmed() == "OK";
}

QStringList PythonEnvironmentManager::GetInstalledPackages(const QString& venv_path)
{
  QString output = RunPipCommand({"list", "--format=freeze"}, venv_path, 30000);
  return output.split('\n', Qt::SkipEmptyParts);
}

bool PythonEnvironmentManager::VerifyPluginDependencies(const QString& plugin_id,
                                                        const QString& venv_path)
{
  QString python_path = GetPythonPath(venv_path);

  if (plugin_id == "detectron2")
  {
    QString check_script =
        "import sys\n"
        "try:\n"
        "    import torch\n"
        "    import detectron2\n"
        "    from detectron2 import model_zoo\n"
        "    from detectron2.config import get_cfg\n"
        "    print('OK')\n"
        "except ImportError as e:\n"
        "    print(f'FAIL: {e}')\n"
        "    sys.exit(1)\n";
    QString output = RunPythonCommand(python_path, {"-c", check_script}, 30000);
    return output.trimmed().startsWith("OK");
  }
  else if (plugin_id == "smp")
  {
    QString check_script =
        "import sys\n"
        "try:\n"
        "    import torch\n"
        "    import segmentation_models_pytorch as smp\n"
        "    print('OK')\n"
        "except ImportError as e:\n"
        "    print(f'FAIL: {e}')\n"
        "    sys.exit(1)\n";
    QString output = RunPythonCommand(python_path, {"-c", check_script}, 30000);
    return output.trimmed().startsWith("OK");
  }

  return python_info_.is_valid;
}

void PythonEnvironmentManager::Cancel()
{
  if (current_process_ && current_process_->state() != QProcess::NotRunning)
  {
    current_process_->kill();
    current_process_->waitForFinished(5000);
  }
  current_operation_ = OperationType::None;
}

bool PythonEnvironmentManager::IsBusy() const
{
  return current_operation_ != OperationType::None;
}

void PythonEnvironmentManager::OnProcessFinished(int exit_code, QProcess::ExitStatus exit_status)
{
  Q_UNUSED(exit_code)
  Q_UNUSED(exit_status)
}

void PythonEnvironmentManager::OnProcessError(QProcess::ProcessError error)
{
  Q_UNUSED(error)
}

void PythonEnvironmentManager::OnProcessOutput()
{
}

QString PythonEnvironmentManager::RunPythonCommand(const QString& python_path,
                                                   const QStringList& args, int timeout_ms)
{
  QProcess process;
  process.setProgram(python_path);
  process.setArguments(args);
  process.start();

  if (!process.waitForFinished(timeout_ms))
  {
    process.kill();
    return QString();
  }

  if (process.exitCode() != 0)
  {
    return QString();
  }

  return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

QString PythonEnvironmentManager::RunPipCommand(const QStringList& args, const QString& venv_path,
                                                int timeout_ms)
{
  QString pip_path = GetPipPath(venv_path);

  QProcess process;
  process.setProgram(pip_path);
  process.setArguments(args);
  process.start();

  if (!process.waitForFinished(timeout_ms))
  {
    process.kill();
    return QString();
  }

  return QString::fromUtf8(process.readAllStandardOutput());
}

void PythonEnvironmentManager::StartAsyncProcess(const QString& program, const QStringList& args)
{
  if (current_process_)
  {
    delete current_process_;
  }

  current_process_ = new QProcess(this);
  connect(current_process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &PythonEnvironmentManager::OnProcessFinished);
  connect(current_process_, &QProcess::errorOccurred, this,
          &PythonEnvironmentManager::OnProcessError);
  connect(current_process_, &QProcess::readyReadStandardOutput, this,
          &PythonEnvironmentManager::OnProcessOutput);

  current_process_->setProgram(program);
  current_process_->setArguments(args);
  current_process_->start();
}

QString PythonEnvironmentManager::GetPipPath(const QString& venv_path) const
{
  if (venv_path.isEmpty())
  {
#ifdef Q_OS_WIN
    return "pip";
#else
    return "pip3";
#endif
  }
  return GetVenvPipPath(venv_path);
}

QString PythonEnvironmentManager::GetPythonPath(const QString& venv_path) const
{
  if (venv_path.isEmpty())
  {
    return python_info_.path;
  }
  return GetVenvPythonPath(venv_path);
}

void PythonEnvironmentManager::ParsePythonVersion(const QString& version_string)
{
  QRegularExpression version_regex(R"(Python\s+(\d+)\.(\d+)\.?(\d*))");
  QRegularExpressionMatch match = version_regex.match(version_string);

  if (match.hasMatch())
  {
    python_info_.version_major = match.captured(1).toInt();
    python_info_.version_minor = match.captured(2).toInt();
    int patch = match.captured(3).isEmpty() ? 0 : match.captured(3).toInt();
    python_info_.version =
        QString("%1.%2.%3").arg(python_info_.version_major).arg(python_info_.version_minor).arg(patch);
  }
}

bool PythonEnvironmentManager::CheckModuleAvailable(const QString& python_path,
                                                    const QString& module_name)
{
  QString output =
      RunPythonCommand(python_path, {"-c", QString("import %1; print('OK')").arg(module_name)}, 5000);
  return output.trimmed() == "OK";
}

void PythonEnvironmentManager::DetectCudaSupport(const QString& python_path)
{
  QString check_script =
      "import sys\n"
      "try:\n"
      "    import torch\n"
      "    print(f'TORCH:{torch.__version__}')\n"
      "    if torch.cuda.is_available():\n"
      "        print(f'CUDA:{torch.version.cuda}')\n"
      "        print(f'GPU:{torch.cuda.get_device_name(0)}')\n"
      "    else:\n"
      "        print('CUDA:NONE')\n"
      "except ImportError:\n"
      "    print('TORCH:NONE')\n"
      "    print('CUDA:NONE')\n";

  QString output = RunPythonCommand(python_path, {"-c", check_script}, 15000);

  for (const QString& line : output.split('\n'))
  {
    if (line.startsWith("TORCH:") && !line.contains("NONE"))
    {
      python_info_.torch_version = line.mid(6).trimmed();
    }
    else if (line.startsWith("CUDA:") && !line.contains("NONE"))
    {
      python_info_.has_cuda = true;
      python_info_.cuda_version = line.mid(5).trimmed();
    }
  }
}

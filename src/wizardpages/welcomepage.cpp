#include "welcomepage.h"

#include <QProcess>

#include "../pluginwizard.h"
#include "ui_welcomepage.h"

WelcomePage::WelcomePage(PluginWizard* wizard)
    : QWizardPage(wizard), wizard_(wizard), ui_(new Ui::WelcomePage)
{
  ui_->setupUi(this);

  setTitle(tr("Welcome to the AI Plugin Setup Wizard"));
  setSubTitle(
      tr("This wizard will help you configure an AI plugin for automatic "
         "segmentation detection in your project."));
}

WelcomePage::~WelcomePage()
{
  delete ui_;
}

void WelcomePage::initializePage()
{
  DetectPythonEnvironment();
  ui_->python_info_label_->setText(FormatPythonInfo());
}

void WelcomePage::DetectPythonEnvironment()
{
  PluginWizard::PythonInfo info;
  info.has_venv = false;
  info.has_pip = false;
  info.has_cuda = false;
  info.has_mps = false;

  // Detect Python path and version
  QProcess process;
  process.start("python3", {"--version"});
  process.waitForFinished(5000);

  if (process.exitCode() == 0)
  {
    QString output = process.readAllStandardOutput().trimmed();
    // Output is like "Python 3.11.2"
    QStringList parts = output.split(' ');
    if (parts.size() >= 2)
    {
      info.version = parts[1];
    }

    // Get Python path
    QProcess which_process;
    which_process.start("which", {"python3"});
    which_process.waitForFinished(5000);
    if (which_process.exitCode() == 0)
    {
      info.path = which_process.readAllStandardOutput().trimmed();
    }

    // Check for venv module
    QProcess venv_process;
    venv_process.start("python3", {"-c", "import venv; print('OK')"});
    venv_process.waitForFinished(5000);
    info.has_venv = (venv_process.exitCode() == 0);

    // Check for pip
    QProcess pip_process;
    pip_process.start("python3", {"-m", "pip", "--version"});
    pip_process.waitForFinished(5000);
    info.has_pip = (pip_process.exitCode() == 0);

    // Check for CUDA via PyTorch
    QProcess cuda_process;
    cuda_process.start("python3",
                       {"-c",
                        "import torch; print('CUDA' if torch.cuda.is_available() else 'NO'); "
                        "print(torch.cuda.get_device_name(0) if torch.cuda.is_available() "
                        "else '')"});
    cuda_process.waitForFinished(10000);

    if (cuda_process.exitCode() == 0)
    {
      QString cuda_output = cuda_process.readAllStandardOutput().trimmed();
      QStringList cuda_lines = cuda_output.split('\n');
      if (!cuda_lines.isEmpty() && cuda_lines[0] == "CUDA")
      {
        info.has_cuda = true;
        if (cuda_lines.size() > 1)
        {
          info.cuda_version = cuda_lines[1];
        }
      }
    }

    // Check for Apple Silicon MPS via PyTorch
    QProcess mps_process;
    mps_process.start("python3",
                      {"-c",
                       "import torch; print('MPS' if (hasattr(torch.backends, 'mps') and "
                       "torch.backends.mps.is_available()) else 'NO')"});
    mps_process.waitForFinished(10000);

    if (mps_process.exitCode() == 0)
    {
      QString mps_output = mps_process.readAllStandardOutput().trimmed();
      if (mps_output == "MPS")
      {
        info.has_mps = true;
      }
    }
  }

  wizard_->SetPythonInfo(info);
}

QString WelcomePage::FormatPythonInfo() const
{
  PluginWizard::PythonInfo info = wizard_->GetPythonInfo();

  QString text;

  if (info.path.isEmpty())
  {
    text = tr("Python: Not detected\n\n"
              "Please install Python 3.8+ to use AI plugins.\n"
              "Visit: https://www.python.org/downloads/");
  }
  else
  {
    text = tr("Python: %1 (%2)\n").arg(info.path).arg(info.version);

    if (info.has_cuda)
    {
      text += tr("CUDA: Available (%1)\n").arg(info.cuda_version);
    }
    else
    {
      text += tr("CUDA: Not available\n");
    }

    if (info.has_mps)
    {
      text += tr("Apple Silicon (MPS): Available\n");
    }
    else
    {
      text += tr("Apple Silicon (MPS): Not available\n");
    }

    if (!info.has_cuda && !info.has_mps)
    {
      text += tr("Note: CPU will be used for inference\n");
    }

    if (!info.has_pip)
    {
      text += tr("\nWarning: pip not found. Package installation may fail.");
    }

    if (!info.has_venv)
    {
      text += tr("\nWarning: venv module not found. Virtual environment creation disabled.");
    }
  }

  return text;
}

bool WelcomePage::validatePage()
{
  // Python is not strictly required for custom binary plugins
  // but we'll allow proceeding with a warning
  return true;
}

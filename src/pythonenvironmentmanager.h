#ifndef PYTHONENVIRONMENTMANAGER_H
#define PYTHONENVIRONMENTMANAGER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

/**
 * @brief Information about the detected Python environment
 */
struct PythonInfo
{
  QString path;           // Full path to python executable (e.g., /usr/bin/python3)
  QString version;        // Python version string (e.g., "3.11.2")
  int version_major;      // Major version number (e.g., 3)
  int version_minor;      // Minor version number (e.g., 11)
  bool has_venv;          // Whether venv module is available
  bool has_pip;           // Whether pip is available
  bool has_cuda;          // Whether CUDA is available via PyTorch
  QString cuda_version;   // CUDA version if available
  QString torch_version;  // PyTorch version if installed
  bool is_valid;          // Whether Python was successfully detected

  PythonInfo();
  QString GetDisplayString() const;
  bool MeetsMinimumRequirements() const;
};

/**
 * @brief Result of a pip installation operation
 */
struct InstallationResult
{
  bool success;
  QString output;
  QString error_message;
  QStringList installed_packages;
  QStringList failed_packages;
};

/**
 * @brief Manages Python environment detection, venv creation, and package installation
 *
 * Handles:
 * - Detecting installed Python interpreters
 * - Checking for CUDA/GPU support
 * - Creating virtual environments for projects
 * - Installing pip packages and requirements files
 * - Verifying plugin installations
 */
class PythonEnvironmentManager : public QObject
{
  Q_OBJECT

 public:
  explicit PythonEnvironmentManager(QObject* parent = nullptr);
  ~PythonEnvironmentManager() override;

  /**
   * @brief Detect the system Python installation
   * @return PythonInfo struct with detected information
   *
   * Checks for python3 first, then python. Detects version, venv support,
   * pip availability, and CUDA support.
   */
  PythonInfo DetectPython();

  /**
   * @brief Get the last detected Python info
   * @return Previously detected PythonInfo
   */
  const PythonInfo& GetPythonInfo() const { return python_info_; }

  /**
   * @brief Check if Python detection has been performed
   * @return true if DetectPython() has been called
   */
  bool IsDetected() const { return detection_performed_; }

  /**
   * @brief Get the path to a project's virtual environment
   * @param project_dir Project directory path
   * @return Path to .venv directory within the project
   */
  static QString GetVenvPath(const QString& project_dir);

  /**
   * @brief Get the Python executable path within a venv
   * @param venv_path Path to the virtual environment
   * @return Path to python executable in venv
   */
  static QString GetVenvPythonPath(const QString& venv_path);

  /**
   * @brief Get the pip executable path within a venv
   * @param venv_path Path to the virtual environment
   * @return Path to pip executable in venv
   */
  static QString GetVenvPipPath(const QString& venv_path);

  /**
   * @brief Get the activation command for a venv
   * @param venv_path Path to the virtual environment
   * @return Shell command to activate the venv
   */
  static QString GetVenvActivateCommand(const QString& venv_path);

  /**
   * @brief Check if a project has a virtual environment
   * @param project_dir Project directory path
   * @return true if .venv exists and appears valid
   */
  static bool HasProjectVenv(const QString& project_dir);

  /**
   * @brief Create a virtual environment for a project
   * @param project_dir Project directory path
   *
   * Emits venvCreationProgress and venvCreationFinished signals.
   * Creates .venv subdirectory with isolated Python environment.
   */
  void CreateProjectVenv(const QString& project_dir);

  /**
   * @brief Install packages from a requirements file
   * @param requirements_file Path to requirements.txt file
   * @param venv_path Optional venv path (uses system pip if empty)
   *
   * Emits installationProgress during installation and
   * installationFinished when complete.
   */
  void InstallRequirements(const QString& requirements_file, const QString& venv_path = QString());

  /**
   * @brief Install a single package via pip
   * @param package_name Package name (e.g., "torch", "numpy>=1.20")
   * @param venv_path Optional venv path (uses system pip if empty)
   */
  void InstallPackage(const QString& package_name, const QString& venv_path = QString());

  /**
   * @brief Install multiple packages via pip
   * @param packages List of package names
   * @param venv_path Optional venv path
   */
  void InstallPackages(const QStringList& packages, const QString& venv_path = QString());

  /**
   * @brief Check if a package is installed
   * @param package_name Package name (without version specifier)
   * @param venv_path Optional venv path
   * @return true if package is importable
   */
  bool IsPackageInstalled(const QString& package_name, const QString& venv_path = QString());

  /**
   * @brief Get list of installed packages
   * @param venv_path Optional venv path
   * @return List of "package==version" strings
   */
  QStringList GetInstalledPackages(const QString& venv_path = QString());

  /**
   * @brief Verify that a plugin's dependencies are installed
   * @param plugin_id Plugin identifier (e.g., "detectron2", "smp")
   * @param venv_path Optional venv path
   * @return true if plugin can be imported
   */
  bool VerifyPluginDependencies(const QString& plugin_id, const QString& venv_path = QString());

  /**
   * @brief Cancel any running operation
   */
  void Cancel();

  /**
   * @brief Check if an operation is in progress
   * @return true if installing or creating venv
   */
  bool IsBusy() const;

 signals:
  /**
   * @brief Emitted during Python detection
   * @param step Description of current detection step
   */
  void detectionProgress(const QString& step);

  /**
   * @brief Emitted when Python detection is complete
   * @param info Detected Python information
   */
  void detectionFinished(const PythonInfo& info);

  /**
   * @brief Emitted during venv creation
   * @param message Progress message
   */
  void venvCreationProgress(const QString& message);

  /**
   * @brief Emitted when venv creation is complete
   * @param success true if venv was created successfully
   * @param venv_path Path to created venv (empty on failure)
   * @param error_message Error description if failed
   */
  void venvCreationFinished(bool success, const QString& venv_path, const QString& error_message);

  /**
   * @brief Emitted during pip installation
   * @param message Progress message (package being installed, etc.)
   * @param progress Progress percentage (0-100), -1 if indeterminate
   */
  void installationProgress(const QString& message, int progress);

  /**
   * @brief Emitted when installation is complete
   * @param result Detailed installation result
   */
  void installationFinished(const InstallationResult& result);

  /**
   * @brief Emitted when an error occurs
   * @param error_message Description of the error
   */
  void errorOccurred(const QString& error_message);

 private slots:
  void OnProcessFinished(int exit_code, QProcess::ExitStatus exit_status);
  void OnProcessError(QProcess::ProcessError error);
  void OnProcessOutput();

 private:
  QString RunPythonCommand(const QString& python_path, const QStringList& args, int timeout_ms = 30000);
  QString RunPipCommand(const QStringList& args, const QString& venv_path = QString(),
                        int timeout_ms = 300000);
  void StartAsyncProcess(const QString& program, const QStringList& args);
  QString GetPipPath(const QString& venv_path = QString()) const;
  QString GetPythonPath(const QString& venv_path = QString()) const;
  void ParsePythonVersion(const QString& version_string);
  bool CheckModuleAvailable(const QString& python_path, const QString& module_name);
  void DetectCudaSupport(const QString& python_path);

  PythonInfo python_info_;
  bool detection_performed_;
  QProcess* current_process_;

  enum class OperationType
  {
    None,
    Detection,
    VenvCreation,
    Installation
  };
  OperationType current_operation_;
  QString current_venv_path_;
  QString current_requirements_file_;
};

#endif  // PYTHONENVIRONMENTMANAGER_H

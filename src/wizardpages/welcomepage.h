#ifndef WELCOMEPAGE_H
#define WELCOMEPAGE_H

#include <QWizardPage>

namespace Ui
{
class WelcomePage;
}

class PluginWizard;

/**
 * @brief Welcome page of the Plugin Wizard
 *
 * Displays:
 * - Introduction to the wizard
 * - Detected Python environment
 * - CUDA/GPU availability
 * - Option to create virtual environment
 */
class WelcomePage : public QWizardPage
{
  Q_OBJECT

 public:
  explicit WelcomePage(PluginWizard* wizard);
  ~WelcomePage() override;

  void initializePage() override;
  bool validatePage() override;

 private:
  void DetectPythonEnvironment();
  QString FormatPythonInfo() const;

  PluginWizard* wizard_;
  Ui::WelcomePage* ui_;
};

#endif  // WELCOMEPAGE_H

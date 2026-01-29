#ifndef CUSTOMPLUGINPAGE_H
#define CUSTOMPLUGINPAGE_H

#include <QWizardPage>

namespace Ui
{
class CustomPluginPage;
}

class PluginWizard;

/**
 * @brief Custom plugin configuration page
 *
 * Allows user to configure:
 * - Plugin command (required)
 * - Requirements file (optional)
 * - Environment setup command (optional)
 * - Plugin display name
 * - Virtual environment option
 */
class CustomPluginPage : public QWizardPage
{
  Q_OBJECT

 public:
  explicit CustomPluginPage(PluginWizard* wizard);
  ~CustomPluginPage() override;

  void initializePage() override;
  bool validatePage() override;
  bool isComplete() const override;

 private slots:
  void OnBrowseRequirements();
  void OnClearRequirements();
  void OnCommandChanged();

 private:
  void ConnectSignals();

  PluginWizard* wizard_;
  Ui::CustomPluginPage* ui_;
};

#endif  // CUSTOMPLUGINPAGE_H

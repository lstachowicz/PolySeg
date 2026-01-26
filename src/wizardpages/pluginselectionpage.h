#ifndef PLUGINSELECTIONPAGE_H
#define PLUGINSELECTIONPAGE_H

#include <QButtonGroup>
#include <QWizardPage>

namespace Ui
{
class PluginSelectionPage;
}

class PluginWizard;

/**
 * @brief Plugin selection page of the Plugin Wizard
 *
 * Allows user to choose between:
 * - Detectron2 (Meta AI's detection library)
 * - SMP (Segmentation Models PyTorch)
 * - Custom Plugin (user-provided executable)
 */
class PluginSelectionPage : public QWizardPage
{
  Q_OBJECT

 public:
  explicit PluginSelectionPage(PluginWizard* wizard);
  ~PluginSelectionPage() override;

  void initializePage() override;
  bool validatePage() override;
  bool isComplete() const override;

 private slots:
  void OnPluginSelected(int id);

 private:
  void SetupButtonGroup();

  PluginWizard* wizard_;
  Ui::PluginSelectionPage* ui_;
  QButtonGroup* plugin_group_;
};

#endif  // PLUGINSELECTIONPAGE_H

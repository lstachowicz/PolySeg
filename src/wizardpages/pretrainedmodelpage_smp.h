#ifndef PRETRAINEDMODELPAGE_SMP_H
#define PRETRAINEDMODELPAGE_SMP_H

#include <QButtonGroup>
#include <QWizardPage>

namespace Ui
{
class PretrainedModelPage_SMP;
}

class PluginWizard;

/**
 * @brief Pre-trained model selection page for Segmentation Models PyTorch (SMP)
 *
 * Options:
 * - Use ImageNet pretrained encoder weights (recommended)
 * - Start from scratch (random weights)
 * - Use existing model file (file browser)
 */
class PretrainedModelPage_SMP : public QWizardPage
{
  Q_OBJECT

 public:
  explicit PretrainedModelPage_SMP(PluginWizard* wizard);
  ~PretrainedModelPage_SMP() override;

  void initializePage() override;
  bool validatePage() override;
  bool isComplete() const override;

 private slots:
  void OnModeChanged(int id);
  void OnBrowseExistingModel();

 private:
  void SetupConnections();
  void UpdateVisibility();

  Ui::PretrainedModelPage_SMP* ui_;
  PluginWizard* wizard_;
  QButtonGroup* mode_group_;
};

#endif  // PRETRAINEDMODELPAGE_SMP_H

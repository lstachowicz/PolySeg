#ifndef PRETRAINEDMODELPAGE_DETECTRON2_H
#define PRETRAINEDMODELPAGE_DETECTRON2_H

#include <QButtonGroup>
#include <QWizardPage>

namespace Ui
{
class PretrainedModelPage_Detectron2;
}

class PluginWizard;

/**
 * @brief Pre-trained model selection page for Detectron2
 *
 * Options:
 * - Download pre-trained model (COCO, LVIS, Cityscapes)
 * - Start from scratch (random weights)
 * - Use existing model file (file browser)
 */
class PretrainedModelPage_Detectron2 : public QWizardPage
{
  Q_OBJECT

 public:
  explicit PretrainedModelPage_Detectron2(PluginWizard* wizard);
  ~PretrainedModelPage_Detectron2() override;

  void initializePage() override;
  bool validatePage() override;
  bool isComplete() const override;

 private slots:
  void OnModeChanged(int id);
  void OnModelSelected(int id);
  void OnBrowseExistingModel();

 private:
  void SetupConnections();
  void UpdateVisibility();

  Ui::PretrainedModelPage_Detectron2* ui_;
  PluginWizard* wizard_;
  QButtonGroup* mode_group_;
  QButtonGroup* model_group_;
};

#endif  // PRETRAINEDMODELPAGE_DETECTRON2_H

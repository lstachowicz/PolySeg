#ifndef MODELSELECTIONPAGE_DETECTRON2_H
#define MODELSELECTIONPAGE_DETECTRON2_H

#include <QButtonGroup>
#include <QWizardPage>

namespace Ui
{
class ModelSelectionPage_Detectron2;
}

class PluginWizard;

/**
 * @brief Model architecture selection page for Detectron2
 *
 * Options:
 * - Architecture: Mask R-CNN, Cascade Mask R-CNN
 * - Backbone: ResNet-50-FPN, ResNet-101-FPN, ResNeXt-101-FPN
 */
class ModelSelectionPage_Detectron2 : public QWizardPage
{
  Q_OBJECT

 public:
  explicit ModelSelectionPage_Detectron2(PluginWizard* wizard);
  ~ModelSelectionPage_Detectron2() override;

  void initializePage() override;
  bool validatePage() override;

 private slots:
  void OnArchitectureChanged(int id);
  void OnBackboneChanged(int index);
  void UpdateModelEstimates();

 private:
  void SetupConnections();
  void PopulateBackbones();

  Ui::ModelSelectionPage_Detectron2* ui_;
  PluginWizard* wizard_;
  QButtonGroup* arch_group_;
};

#endif  // MODELSELECTIONPAGE_DETECTRON2_H

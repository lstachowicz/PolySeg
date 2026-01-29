#ifndef MODELSELECTIONPAGE_SMP_H
#define MODELSELECTIONPAGE_SMP_H

#include <QWizardPage>

namespace Ui
{
class ModelSelectionPage_SMP;
}

class PluginWizard;

/**
 * @brief Model architecture selection page for Segmentation Models PyTorch (SMP)
 *
 * Options:
 * - Decoder: UNet, UNet++, DeepLabV3+, FPN, PSPNet, MAnet
 * - Encoder: resnet34, resnet50, efficientnet-b0..b7, mobilenet_v2
 * - Pretrained encoder weights option
 */
class ModelSelectionPage_SMP : public QWizardPage
{
  Q_OBJECT

 public:
  explicit ModelSelectionPage_SMP(PluginWizard* wizard);
  ~ModelSelectionPage_SMP() override;

  void initializePage() override;
  bool validatePage() override;

 private slots:
  void UpdateModelEstimates();

 private:
  void SetupConnections();
  void PopulateDecoders();
  void PopulateEncoders();

  Ui::ModelSelectionPage_SMP* ui_;
  PluginWizard* wizard_;
};

#endif  // MODELSELECTIONPAGE_SMP_H

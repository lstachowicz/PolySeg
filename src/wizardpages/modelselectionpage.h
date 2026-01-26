#ifndef MODELSELECTIONPAGE_H
#define MODELSELECTIONPAGE_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QRadioButton>
#include <QWizardPage>

namespace Ui
{
class ModelSelectionPage;
}

class PluginWizard;

/**
 * @brief Model architecture selection page
 *
 * For Detectron2:
 * - Architecture: Mask R-CNN, Cascade Mask R-CNN
 * - Backbone: ResNet-50-FPN, ResNet-101-FPN, ResNeXt-101-FPN
 *
 * For SMP:
 * - Decoder: UNet, UNet++, DeepLabV3+, FPN, PSPNet, MAnet
 * - Encoder: resnet34, resnet50, efficientnet-b0..b7, mobilenet_v2
 */
class ModelSelectionPage : public QWizardPage
{
  Q_OBJECT

 public:
  explicit ModelSelectionPage(PluginWizard* wizard);
  ~ModelSelectionPage() override;

  void initializePage() override;
  bool validatePage() override;

 private slots:
  void OnArchitectureChanged(int id);
  void OnBackboneChanged(int index);
  void UpdateModelEstimates();

 private:
  void SetupDetectron2UI();
  void SetupSmpUI();
  void ClearLayout();

  Ui::ModelSelectionPage* ui_;
  PluginWizard* wizard_;

  // Detectron2 widgets
  QButtonGroup* arch_group_;
  QRadioButton* mask_rcnn_radio_;
  QRadioButton* cascade_radio_;
  QComboBox* backbone_combo_;

  // SMP widgets
  QComboBox* decoder_combo_;
  QComboBox* encoder_combo_;
  QCheckBox* pretrained_encoder_checkbox_;
};

#endif  // MODELSELECTIONPAGE_H

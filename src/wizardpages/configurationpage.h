#ifndef CONFIGURATIONPAGE_H
#define CONFIGURATIONPAGE_H

#include <QButtonGroup>
#include <QWizardPage>

namespace Ui
{
class ConfigurationPage;
}

class PluginWizard;

/**
 * @brief Detection configuration page
 *
 * Settings:
 * - Confidence threshold (slider + value)
 * - NMS IoU threshold
 * - Device selection (Auto/CPU/GPU)
 * - Advanced settings (min object size, max detections, image size)
 */
class ConfigurationPage : public QWizardPage
{
  Q_OBJECT

 public:
  explicit ConfigurationPage(PluginWizard* wizard);
  ~ConfigurationPage() override;

  void initializePage() override;
  bool validatePage() override;

 private slots:
  void OnConfidenceSliderChanged(int value);
  void OnNmsSliderChanged(int value);
  void OnDeviceChanged(int id);

 private:
  void SetupButtonGroup();
  void ConnectSignals();
  void UpdateDeviceAvailability();

  PluginWizard* wizard_;
  Ui::ConfigurationPage* ui_;
  QButtonGroup* device_group_;
};

#endif  // CONFIGURATIONPAGE_H

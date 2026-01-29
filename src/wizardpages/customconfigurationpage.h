#ifndef CUSTOMCONFIGURATIONPAGE_H
#define CUSTOMCONFIGURATIONPAGE_H

#include <QMap>
#include <QWizardPage>

namespace Ui
{
class CustomConfigurationPage;
}

class PluginWizard;

/**
 * @brief Custom plugin configuration page
 *
 * Allows user to configure:
 * - Detection arguments with placeholders
 * - Training arguments (optional)
 * - Custom settings (key-value pairs)
 * - Plugin test button
 */
class CustomConfigurationPage : public QWizardPage
{
  Q_OBJECT

 public:
  explicit CustomConfigurationPage(PluginWizard* wizard);
  ~CustomConfigurationPage() override;

  void initializePage() override;
  bool validatePage() override;

 private slots:
  void OnAddSetting();
  void OnRemoveSetting();
  void OnTestPlugin();

 private:
  void PopulateSettingsTable();
  QMap<QString, QString> GetSettingsFromTable() const;

  Ui::CustomConfigurationPage* ui_;
  PluginWizard* wizard_;
};

#endif  // CUSTOMCONFIGURATIONPAGE_H

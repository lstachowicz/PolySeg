#ifndef PRETRAINEDMODELPAGE_H
#define PRETRAINEDMODELPAGE_H

#include <QButtonGroup>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QWizardPage>

namespace Ui
{
class PretrainedModelPage;
}

class PluginWizard;

/**
 * @brief Pre-trained model selection page
 *
 * Options:
 * - Download pre-trained model (with selection of available models)
 * - Start from scratch (random weights)
 * - Use existing model file (file browser)
 */
class PretrainedModelPage : public QWizardPage
{
  Q_OBJECT

 public:
  explicit PretrainedModelPage(PluginWizard* wizard);
  ~PretrainedModelPage() override;

  void initializePage() override;
  bool validatePage() override;
  bool isComplete() const override;

 private slots:
  void OnModeChanged(int id);
  void OnModelSelected(int id);
  void OnBrowseExistingModel();

 private:
  void PopulatePretrainedModels();

  Ui::PretrainedModelPage* ui_;
  PluginWizard* wizard_;

  // Mode selection
  QButtonGroup* mode_group_;

  // Pre-trained model selection
  QButtonGroup* model_group_;
};

#endif  // PRETRAINEDMODELPAGE_H

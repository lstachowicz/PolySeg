#ifndef SUMMARYPAGE_H
#define SUMMARYPAGE_H

#include <QWizardPage>

namespace Ui
{
class SummaryPage;
}

class PluginWizard;

/**
 * @brief Summary page showing final configuration
 *
 * Displays:
 * - All selected options
 * - Configuration summary
 * - Next steps guide
 * - Option to run test detection
 */
class SummaryPage : public QWizardPage
{
  Q_OBJECT

 public:
  explicit SummaryPage(PluginWizard* wizard);
  ~SummaryPage() override;

  void initializePage() override;
  bool validatePage() override;

 private:
  QString GenerateSummary() const;

  PluginWizard* wizard_;
  Ui::SummaryPage* ui_;
};

#endif  // SUMMARYPAGE_H

#ifndef DOWNLOADPAGE_H
#define DOWNLOADPAGE_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWizardPage>

namespace Ui
{
class DownloadPage;
}

class PluginWizard;

/**
 * @brief Model download page with progress tracking
 *
 * Handles:
 * - Downloading pre-trained model from URL
 * - Progress bar and download statistics
 * - Checksum verification
 * - Python dependencies installation (optional)
 */
class DownloadPage : public QWizardPage
{
  Q_OBJECT

 public:
  explicit DownloadPage(PluginWizard* wizard);
  ~DownloadPage() override;

  void initializePage() override;
  bool validatePage() override;
  bool isComplete() const override;

 private slots:
  void OnDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
  void OnDownloadFinished();
  void OnDownloadError(QNetworkReply::NetworkError error);
  void OnCancelDownload();

 private:
  void ConnectSignals();
  void StartDownload();
  QString GetDownloadUrl() const;
  QString GetDestinationPath() const;
  QString FormatBytes(qint64 bytes) const;
  QString FormatSpeed(qint64 bytesPerSecond) const;

  PluginWizard* wizard_;
  Ui::DownloadPage* ui_;

  // Network
  QNetworkAccessManager* network_manager_;
  QNetworkReply* current_reply_;

  // State
  bool download_complete_;
  bool download_cancelled_;
  qint64 download_start_time_;
  qint64 last_bytes_received_;
  qint64 last_time_;
};

#endif  // DOWNLOADPAGE_H

#include "downloadpage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>

#include "../pluginwizard.h"
#include "ui_downloadpage.h"

DownloadPage::DownloadPage(PluginWizard* wizard)
    : QWizardPage(wizard),
      wizard_(wizard),
      ui_(new Ui::DownloadPage),
      network_manager_(new QNetworkAccessManager(this)),
      current_reply_(nullptr),
      download_complete_(false),
      download_cancelled_(false),
      download_start_time_(0),
      last_bytes_received_(0),
      last_time_(0)
{
  ui_->setupUi(this);

  setTitle(tr("Downloading Model"));
  setSubTitle(tr("Please wait while the model is being downloaded."));

  ConnectSignals();
}

DownloadPage::~DownloadPage()
{
  if (current_reply_)
  {
    current_reply_->abort();
    current_reply_->deleteLater();
  }
  delete ui_;
}

void DownloadPage::ConnectSignals()
{
  connect(ui_->cancel_button_, &QPushButton::clicked, this, &DownloadPage::OnCancelDownload);
}

void DownloadPage::initializePage()
{
  download_complete_ = false;
  download_cancelled_ = false;

  // Reset UI
  ui_->progress_bar_->setValue(0);
  ui_->progress_label_->setText(tr("Downloaded: 0 MB / 0 MB"));
  ui_->speed_label_->setText(tr("Speed: -- MB/s"));
  ui_->remaining_label_->setText(tr("Remaining: --"));

  ui_->deps_checkbox_->setChecked(false);
  ui_->verify_checkbox_->setChecked(false);
  ui_->test_checkbox_->setChecked(false);

  ui_->cancel_button_->setEnabled(true);

  // Set filename and destination
  QString model_id = wizard_->GetSelectedModelId();
  ui_->filename_label_->setText(tr("Downloading: %1").arg(model_id));
  ui_->destination_label_->setText(GetDestinationPath());

  // Start download
  StartDownload();
}

void DownloadPage::StartDownload()
{
  QString url = GetDownloadUrl();

  if (url.isEmpty())
  {
    // No download needed (e.g., SMP uses PyTorch auto-download)
    download_complete_ = true;
    ui_->progress_bar_->setValue(100);
    ui_->progress_label_->setText(tr("No download required"));
    ui_->deps_checkbox_->setChecked(true);
    ui_->verify_checkbox_->setChecked(true);
    ui_->test_checkbox_->setChecked(true);
    ui_->cancel_button_->setEnabled(false);
    emit completeChanged();
    return;
  }

  // Create models directory
  QString dest_path = GetDestinationPath();
  QDir().mkpath(QFileInfo(dest_path).absolutePath());

  download_start_time_ = QDateTime::currentMSecsSinceEpoch();
  last_time_ = download_start_time_;
  last_bytes_received_ = 0;

  QUrl download_url(url);
  QNetworkRequest request{download_url};
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::NoLessSafeRedirectPolicy);

  current_reply_ = network_manager_->get(request);

  connect(current_reply_, &QNetworkReply::downloadProgress, this,
          &DownloadPage::OnDownloadProgress);
  connect(current_reply_, &QNetworkReply::finished, this, &DownloadPage::OnDownloadFinished);
  connect(current_reply_, &QNetworkReply::errorOccurred, this, &DownloadPage::OnDownloadError);
}

QString DownloadPage::GetDownloadUrl() const
{
  QString plugin_id = wizard_->GetSelectedPluginId();
  QString model_id = wizard_->GetSelectedModelId();

  if (plugin_id == "smp" || model_id == "scratch" || model_id == "existing" ||
      model_id == "imagenet_pretrained")
  {
    return QString();  // No download needed
  }

  // Detectron2 model URLs
  QString arch = wizard_->GetSelectedArchitecture();
  QString backbone = wizard_->GetSelectedBackbone();

  // Map to actual Detectron2 model zoo URLs
  if (model_id.startsWith("coco_"))
  {
    if (arch == "mask_rcnn" && backbone == "R_50_FPN")
    {
      return "https://dl.fbaipublicfiles.com/detectron2/COCO-InstanceSegmentation/"
             "mask_rcnn_R_50_FPN_3x/137849600/model_final_f10217.pkl";
    }
    else if (arch == "mask_rcnn" && backbone == "R_101_FPN")
    {
      return "https://dl.fbaipublicfiles.com/detectron2/COCO-InstanceSegmentation/"
             "mask_rcnn_R_101_FPN_3x/138205316/model_final_a3ec72.pkl";
    }
  }

  // Default: return empty for unsupported configurations
  return QString();
}

QString DownloadPage::GetDestinationPath() const
{
  QString project_dir = wizard_->GetProjectDir();
  QString model_id = wizard_->GetSelectedModelId();

  return project_dir + "/models/" + model_id + ".pkl";
}

void DownloadPage::OnDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
  if (bytesTotal <= 0)
  {
    return;
  }

  int percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
  ui_->progress_bar_->setValue(percent);

  ui_->progress_label_->setText(
      tr("Downloaded: %1 / %2").arg(FormatBytes(bytesReceived)).arg(FormatBytes(bytesTotal)));

  // Calculate speed
  qint64 current_time = QDateTime::currentMSecsSinceEpoch();
  qint64 elapsed = current_time - last_time_;

  if (elapsed > 500)
  {  // Update every 500ms
    qint64 bytes_diff = bytesReceived - last_bytes_received_;
    qint64 speed = (bytes_diff * 1000) / elapsed;

    ui_->speed_label_->setText(tr("Speed: %1").arg(FormatSpeed(speed)));

    // Estimate remaining time
    if (speed > 0)
    {
      qint64 remaining_bytes = bytesTotal - bytesReceived;
      qint64 remaining_seconds = remaining_bytes / speed;

      if (remaining_seconds < 60)
      {
        ui_->remaining_label_->setText(tr("Remaining: ~%1 seconds").arg(remaining_seconds));
      }
      else
      {
        ui_->remaining_label_->setText(tr("Remaining: ~%1 minutes").arg(remaining_seconds / 60));
      }
    }

    last_bytes_received_ = bytesReceived;
    last_time_ = current_time;
  }
}

void DownloadPage::OnDownloadFinished()
{
  if (download_cancelled_)
  {
    return;
  }

  if (current_reply_->error() != QNetworkReply::NoError)
  {
    return;  // Error handled by OnDownloadError
  }

  // Save file
  QString dest_path = GetDestinationPath();
  QFile file(dest_path);

  if (file.open(QIODevice::WriteOnly))
  {
    file.write(current_reply_->readAll());
    file.close();

    wizard_->SetModelPath(dest_path);

    // Update status
    ui_->deps_checkbox_->setChecked(true);
    ui_->verify_checkbox_->setChecked(true);
    ui_->test_checkbox_->setChecked(true);

    download_complete_ = true;
    ui_->cancel_button_->setEnabled(false);

    ui_->progress_bar_->setValue(100);
    ui_->remaining_label_->setText(tr("Download complete!"));
  }
  else
  {
    QMessageBox::critical(this, tr("Save Error"),
                          tr("Failed to save model file:\n%1").arg(dest_path));
  }

  current_reply_->deleteLater();
  current_reply_ = nullptr;

  emit completeChanged();
}

void DownloadPage::OnDownloadError(QNetworkReply::NetworkError error)
{
  (void)error;
  if (download_cancelled_)
  {
    return;
  }

  QString error_msg = current_reply_ ? current_reply_->errorString() : tr("Unknown error");

  QMessageBox::critical(this, tr("Download Error"),
                        tr("Failed to download model:\n%1").arg(error_msg));

  ui_->cancel_button_->setEnabled(false);
  ui_->progress_label_->setText(tr("Download failed"));
}

void DownloadPage::OnCancelDownload()
{
  download_cancelled_ = true;

  if (current_reply_)
  {
    current_reply_->abort();
    current_reply_->deleteLater();
    current_reply_ = nullptr;
  }

  ui_->cancel_button_->setEnabled(false);
  ui_->progress_label_->setText(tr("Download cancelled"));
  wizard()->back();
}

QString DownloadPage::FormatBytes(qint64 bytes) const
{
  if (bytes < 1024)
  {
    return QString("%1 B").arg(bytes);
  }
  else if (bytes < 1024 * 1024)
  {
    return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
  }
  else if (bytes < 1024 * 1024 * 1024)
  {
    return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
  }
  else
  {
    return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
  }
}

QString DownloadPage::FormatSpeed(qint64 bytesPerSecond) const
{
  if (bytesPerSecond < 1024)
  {
    return QString("%1 B/s").arg(bytesPerSecond);
  }
  else if (bytesPerSecond < 1024 * 1024)
  {
    return QString("%1 KB/s").arg(bytesPerSecond / 1024.0, 0, 'f', 1);
  }
  else
  {
    return QString("%1 MB/s").arg(bytesPerSecond / (1024.0 * 1024.0), 0, 'f', 1);
  }
}

bool DownloadPage::validatePage()
{
  if (!download_complete_)
  {
    QMessageBox::warning(this, tr("Download In Progress"),
                         tr("Please wait for the download to complete."));
    return false;
  }

  return true;
}

bool DownloadPage::isComplete() const
{
  return download_complete_;
}

#include "modeldownloadmanager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

namespace
{
constexpr int kChecksumBufferSize = 65536;  // 64KB buffer for checksum calculation
}

ModelDownloadManager::ModelDownloadManager(QObject* parent)
    : QObject(parent),
      network_manager_(new QNetworkAccessManager(this)),
      current_reply_(nullptr),
      output_file_(nullptr),
      download_to_cache_(true)
{
}

ModelDownloadManager::~ModelDownloadManager()
{
  CancelDownload();
}

QString ModelDownloadManager::GetGlobalCacheDir()
{
  QString home = QDir::homePath();
  return QDir::cleanPath(home + "/.polyseg/models");
}

QString ModelDownloadManager::GetPluginCacheDir(const QString& plugin_id)
{
  return QDir::cleanPath(GetGlobalCacheDir() + "/" + plugin_id);
}

bool ModelDownloadManager::IsModelCached(const QString& model_id,
                                         const QString& plugin_id) const
{
  QString path = GetCachedModelPath(model_id, plugin_id);
  return !path.isEmpty() && QFile::exists(path);
}

QString ModelDownloadManager::GetCachedModelPath(const QString& model_id,
                                                 const QString& plugin_id) const
{
  QString cache_dir = GetPluginCacheDir(plugin_id);
  QDir dir(cache_dir);
  if (!dir.exists())
  {
    return QString();
  }

  // Look for files starting with model_id
  QStringList filters;
  filters << model_id + ".*" << model_id;

  dir.setNameFilters(filters);
  QStringList files = dir.entryList(QDir::Files);

  if (!files.isEmpty())
  {
    return dir.absoluteFilePath(files.first());
  }

  return QString();
}

void ModelDownloadManager::DownloadModel(const ModelDownloadInfo& info)
{
  download_to_cache_ = true;
  EnsureCacheDirectoryExists(info.plugin_id);

  QString cache_dir = GetPluginCacheDir(info.plugin_id);
  QString filename = GetModelFileName(info.id, info.download_url);
  current_destination_ = QDir::cleanPath(cache_dir + "/" + filename);

  // Check if already cached
  if (QFile::exists(current_destination_))
  {
    emit statusMessage(tr("Model already cached, verifying checksum..."));

    if (!info.checksum_sha256.isEmpty())
    {
      bool valid = VerifyChecksum(current_destination_, info.checksum_sha256);
      emit checksumVerified(valid, current_destination_);

      if (valid)
      {
        emit statusMessage(tr("Using cached model"));
        emit downloadFinished(current_destination_);
        return;
      }
      else
      {
        emit statusMessage(tr("Cached file corrupted, re-downloading..."));
        QFile::remove(current_destination_);
      }
    }
    else
    {
      // No checksum to verify, assume cached file is valid
      emit downloadFinished(current_destination_);
      return;
    }
  }

  // Start download
  current_download_ = info;
  StartDownload();
}

void ModelDownloadManager::DownloadModelToPath(const ModelDownloadInfo& info,
                                               const QString& destination)
{
  download_to_cache_ = false;
  current_destination_ = destination;

  // Ensure parent directory exists
  QFileInfo file_info(destination);
  QDir parent_dir = file_info.absoluteDir();
  if (!parent_dir.exists())
  {
    parent_dir.mkpath(".");
  }

  // Check if already exists
  if (QFile::exists(destination))
  {
    emit statusMessage(tr("File exists, verifying checksum..."));

    if (!info.checksum_sha256.isEmpty())
    {
      bool valid = VerifyChecksum(destination, info.checksum_sha256);
      emit checksumVerified(valid, destination);

      if (valid)
      {
        emit statusMessage(tr("File verified"));
        emit downloadFinished(destination);
        return;
      }
      else
      {
        emit statusMessage(tr("Existing file corrupted, re-downloading..."));
        QFile::remove(destination);
      }
    }
    else
    {
      emit downloadFinished(destination);
      return;
    }
  }

  current_download_ = info;
  StartDownload();
}

void ModelDownloadManager::StartDownload()
{
  if (current_reply_)
  {
    emit downloadError(tr("Download already in progress"));
    return;
  }

  QUrl url(current_download_.download_url);
  if (!url.isValid())
  {
    emit downloadError(tr("Invalid download URL: %1").arg(current_download_.download_url));
    return;
  }

  emit statusMessage(tr("Starting download: %1").arg(current_download_.name));

  // Open output file
  output_file_ = new QFile(current_destination_ + ".part", this);
  if (!output_file_->open(QIODevice::WriteOnly))
  {
    emit downloadError(
        tr("Cannot create output file: %1").arg(output_file_->errorString()));
    delete output_file_;
    output_file_ = nullptr;
    return;
  }

  // Create request
  QNetworkRequest request(url);
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::NoLessSafeRedirectPolicy);

  // Start download
  current_reply_ = network_manager_->get(request);

  connect(current_reply_, &QNetworkReply::downloadProgress, this,
          &ModelDownloadManager::OnDownloadProgress);
  connect(current_reply_, &QNetworkReply::finished, this,
          &ModelDownloadManager::OnDownloadFinished);
  connect(current_reply_, &QNetworkReply::errorOccurred, this,
          &ModelDownloadManager::OnDownloadError);
  connect(current_reply_, &QNetworkReply::sslErrors, this,
          &ModelDownloadManager::OnSslErrors);
  connect(current_reply_, &QNetworkReply::readyRead, this, [this]() {
    if (output_file_ && current_reply_)
    {
      output_file_->write(current_reply_->readAll());
    }
  });
}

void ModelDownloadManager::CancelDownload()
{
  if (current_reply_)
  {
    current_reply_->abort();
    current_reply_->deleteLater();
    current_reply_ = nullptr;
  }

  if (output_file_)
  {
    QString partial_path = output_file_->fileName();
    output_file_->close();
    output_file_->deleteLater();
    output_file_ = nullptr;

    // Remove partial file
    QFile::remove(partial_path);
  }

  emit statusMessage(tr("Download cancelled"));
}

bool ModelDownloadManager::IsDownloading() const
{
  return current_reply_ != nullptr;
}

bool ModelDownloadManager::VerifyChecksum(const QString& file_path,
                                          const QString& expected_sha256)
{
  QString actual = CalculateChecksum(file_path);
  return actual.compare(expected_sha256, Qt::CaseInsensitive) == 0;
}

QString ModelDownloadManager::CalculateChecksum(const QString& file_path)
{
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly))
  {
    return QString();
  }

  QCryptographicHash hash(QCryptographicHash::Sha256);
  char buffer[kChecksumBufferSize];

  while (!file.atEnd())
  {
    qint64 bytes_read = file.read(buffer, kChecksumBufferSize);
    if (bytes_read > 0)
    {
      hash.addData(QByteArrayView(buffer, bytes_read));
    }
  }

  return hash.result().toHex();
}

qint64 ModelDownloadManager::GetCacheSize() const
{
  QString cache_dir = GetGlobalCacheDir();
  qint64 total_size = 0;

  QDirIterator it(cache_dir, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext())
  {
    it.next();
    total_size += it.fileInfo().size();
  }

  return total_size;
}

void ModelDownloadManager::ClearCache()
{
  QString cache_dir = GetGlobalCacheDir();
  QDir dir(cache_dir);

  if (dir.exists())
  {
    dir.removeRecursively();
  }

  emit statusMessage(tr("Cache cleared"));
}

bool ModelDownloadManager::RemoveCachedModel(const QString& model_id,
                                             const QString& plugin_id)
{
  QString path = GetCachedModelPath(model_id, plugin_id);
  if (path.isEmpty())
  {
    return false;
  }

  return QFile::remove(path);
}

void ModelDownloadManager::OnDownloadProgress(qint64 bytes_received, qint64 bytes_total)
{
  emit downloadProgress(bytes_received, bytes_total);
}

void ModelDownloadManager::OnDownloadFinished()
{
  if (!current_reply_ || !output_file_)
  {
    return;
  }

  // Write any remaining data
  if (current_reply_->bytesAvailable() > 0)
  {
    output_file_->write(current_reply_->readAll());
  }

  output_file_->close();

  // Check for errors
  if (current_reply_->error() != QNetworkReply::NoError)
  {
    QString error_msg = current_reply_->errorString();
    QFile::remove(output_file_->fileName());

    output_file_->deleteLater();
    output_file_ = nullptr;
    current_reply_->deleteLater();
    current_reply_ = nullptr;

    emit downloadError(error_msg);
    return;
  }

  // Rename from .part to final name
  QString partial_path = output_file_->fileName();
  output_file_->deleteLater();
  output_file_ = nullptr;

  if (QFile::exists(current_destination_))
  {
    QFile::remove(current_destination_);
  }

  if (!QFile::rename(partial_path, current_destination_))
  {
    emit downloadError(tr("Failed to rename downloaded file"));
    QFile::remove(partial_path);
    current_reply_->deleteLater();
    current_reply_ = nullptr;
    return;
  }

  emit statusMessage(tr("Download complete, verifying checksum..."));

  // Verify checksum if provided
  if (!current_download_.checksum_sha256.isEmpty())
  {
    bool valid = VerifyChecksum(current_destination_, current_download_.checksum_sha256);
    emit checksumVerified(valid, current_destination_);

    if (!valid)
    {
      emit downloadError(tr("Checksum verification failed"));
      QFile::remove(current_destination_);
      current_reply_->deleteLater();
      current_reply_ = nullptr;
      return;
    }
  }

  current_reply_->deleteLater();
  current_reply_ = nullptr;

  emit statusMessage(tr("Model ready: %1").arg(current_download_.name));
  emit downloadFinished(current_destination_);
}

void ModelDownloadManager::OnDownloadError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  if (!current_reply_)
  {
    return;
  }

  QString error_message = current_reply_->errorString();
  emit downloadError(tr("Network error: %1").arg(error_message));
}

void ModelDownloadManager::OnSslErrors(const QList<QSslError>& errors)
{
  QStringList error_strings;
  for (const QSslError& error : errors)
  {
    error_strings << error.errorString();
  }

  emit downloadError(tr("SSL errors: %1").arg(error_strings.join(", ")));
}

void ModelDownloadManager::EnsureCacheDirectoryExists(const QString& plugin_id)
{
  QString cache_dir = GetPluginCacheDir(plugin_id);
  QDir dir(cache_dir);
  if (!dir.exists())
  {
    dir.mkpath(".");
  }
}

QString ModelDownloadManager::GetModelFileName(const QString& model_id,
                                               const QString& url) const
{
  // Extract extension from URL
  QUrl parsed_url(url);
  QString path = parsed_url.path();
  int last_dot = path.lastIndexOf('.');

  QString extension;
  if (last_dot >= 0)
  {
    extension = path.mid(last_dot);
    // Limit extension length
    if (extension.length() > 10)
    {
      extension.clear();
    }
  }

  // Use model_id as base filename
  if (extension.isEmpty())
  {
    return model_id;
  }

  return model_id + extension;
}

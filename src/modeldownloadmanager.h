#ifndef MODELDOWNLOADMANAGER_H
#define MODELDOWNLOADMANAGER_H

#include <QObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

/**
 * @brief Information about a model to download
 */
struct ModelDownloadInfo
{
  QString id;
  QString name;
  QString download_url;
  qint64 size_bytes;
  QString checksum_sha256;
  QString plugin_id;  // "detectron2" or "smp"
};

/**
 * @brief Manages downloading and caching of AI model files
 *
 * Handles:
 * - Global model cache (~/.polyseg/models/)
 * - Download with progress tracking
 * - SHA256 checksum verification
 * - Resume interrupted downloads
 */
class ModelDownloadManager : public QObject
{
  Q_OBJECT

 public:
  explicit ModelDownloadManager(QObject* parent = nullptr);
  ~ModelDownloadManager() override;

  /**
   * @brief Get the global cache directory for models
   * @return Path to ~/.polyseg/models/
   */
  static QString GetGlobalCacheDir();

  /**
   * @brief Get the cache directory for a specific plugin
   * @param plugin_id Plugin identifier (e.g., "detectron2", "smp")
   * @return Path to ~/.polyseg/models/{plugin_id}/
   */
  static QString GetPluginCacheDir(const QString& plugin_id);

  /**
   * @brief Check if a model is already cached
   * @param model_id Unique model identifier
   * @param plugin_id Plugin identifier
   * @return true if model exists in cache
   */
  bool IsModelCached(const QString& model_id, const QString& plugin_id) const;

  /**
   * @brief Get the path to a cached model file
   * @param model_id Unique model identifier
   * @param plugin_id Plugin identifier
   * @return Full path to cached model, empty if not cached
   */
  QString GetCachedModelPath(const QString& model_id, const QString& plugin_id) const;

  /**
   * @brief Start downloading a model
   * @param info Download information including URL and checksum
   *
   * Emits downloadProgress during download and downloadFinished when complete.
   * If model is already cached and valid, emits downloadFinished immediately.
   */
  void DownloadModel(const ModelDownloadInfo& info);

  /**
   * @brief Download a model to a specific destination (project directory)
   * @param info Download information
   * @param destination Target file path
   */
  void DownloadModelToPath(const ModelDownloadInfo& info, const QString& destination);

  /**
   * @brief Cancel the current download
   */
  void CancelDownload();

  /**
   * @brief Check if a download is in progress
   * @return true if currently downloading
   */
  bool IsDownloading() const;

  /**
   * @brief Verify SHA256 checksum of a file
   * @param file_path Path to file
   * @param expected_sha256 Expected checksum (hex string)
   * @return true if checksum matches
   */
  static bool VerifyChecksum(const QString& file_path, const QString& expected_sha256);

  /**
   * @brief Calculate SHA256 checksum of a file
   * @param file_path Path to file
   * @return Checksum as hex string, empty on error
   */
  static QString CalculateChecksum(const QString& file_path);

  /**
   * @brief Get size of cached models
   * @return Total cache size in bytes
   */
  qint64 GetCacheSize() const;

  /**
   * @brief Clear all cached models
   */
  void ClearCache();

  /**
   * @brief Remove a specific cached model
   * @param model_id Model identifier
   * @param plugin_id Plugin identifier
   * @return true if removed successfully
   */
  bool RemoveCachedModel(const QString& model_id, const QString& plugin_id);

 signals:
  /**
   * @brief Emitted during download to report progress
   * @param bytes_received Bytes downloaded so far
   * @param bytes_total Total file size (-1 if unknown)
   */
  void downloadProgress(qint64 bytes_received, qint64 bytes_total);

  /**
   * @brief Emitted when download completes successfully
   * @param file_path Path to downloaded file
   */
  void downloadFinished(const QString& file_path);

  /**
   * @brief Emitted when download fails
   * @param error_message Description of the error
   */
  void downloadError(const QString& error_message);

  /**
   * @brief Emitted after checksum verification
   * @param success true if checksum matches
   * @param file_path Path to verified file
   */
  void checksumVerified(bool success, const QString& file_path);

  /**
   * @brief Emitted with status messages during operations
   * @param message Status message for display
   */
  void statusMessage(const QString& message);

 private slots:
  void OnDownloadProgress(qint64 bytes_received, qint64 bytes_total);
  void OnDownloadFinished();
  void OnDownloadError(QNetworkReply::NetworkError error);
  void OnSslErrors(const QList<QSslError>& errors);

 private:
  void StartDownload();
  void EnsureCacheDirectoryExists(const QString& plugin_id);
  QString GetModelFileName(const QString& model_id, const QString& url) const;

  QNetworkAccessManager* network_manager_;
  QNetworkReply* current_reply_;
  QFile* output_file_;
  ModelDownloadInfo current_download_;
  QString current_destination_;
  bool download_to_cache_;
};

#endif  // MODELDOWNLOADMANAGER_H

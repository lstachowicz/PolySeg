#ifndef METADATAIMPORTER_H
#define METADATAIMPORTER_H

#include <QImage>
#include <QString>
#include <QVector>
#include <memory>

/**
 * @brief Static utility class for importing numerical metadata files as grayscale images
 *
 * Supports importing numerical data arranged in matrix format and converting to
 * grayscale images suitable for annotation in PolySeg. Provides memory-optimized
 * streaming processing for large datasets.
 */
class MetadataImporter
{
public:
    /**
     * @brief Detailed error information for import operations
     */
    struct ImportError
    {
        enum ErrorType
        {
            NO_ERROR,
            FILE_NOT_FOUND,
            INVALID_HEADER_FORMAT,
            INVALID_DIMENSIONS,
            DATA_MISMATCH,
            INVALID_NUMERIC_DATA,
            CROP_BOUNDARY_ERROR,
            FILE_SAVE_ERROR
        } type;

        QString message;
        int row_number;
        int expected_count;
        int actual_count;
        QString invalid_value;
    };

    /**
     * @brief Configuration settings for metadata import operation
     */
    struct ImportSettings
    {
        double range_min;
        double range_max;

        enum OutOfRangeHandling
        {
            CLAMP_TO_BOUNDS,
            SET_TO_ZERO,
            SET_TO_MAX
        } out_of_range_handling;

        // Crop settings - follows existing CropConfig pattern
        bool enable_cropping;
        int crop_start_x;
        int crop_start_y;
        int crop_end_x;
        int crop_end_y;
    };

    /**
     * @brief Import metadata file as grayscale QImage
     * @param filepath Path to metadata file (.txt, .dat, .meta)
     * @param settings Import configuration (range, cropping, etc.)
     * @return Converted grayscale image, null image on error
     */
    static QImage ImportMetadataFile(const QString& filepath,
                                   const ImportSettings& settings);

    /**
     * @brief Import metadata file with detailed error reporting
     * @param filepath Path to metadata file (.txt, .dat, .meta)
     * @param settings Import configuration (range, cropping, etc.)
     * @param error [out] Detailed error information if import fails
     * @return Converted grayscale image, null image on error
     */
    static QImage ImportMetadataFileWithError(const QString& filepath,
                                             const ImportSettings& settings,
                                             ImportError& error);

    /**
     * @brief Parse header to validate format and extract dimensions
     * @param filepath Path to metadata file
     * @param width [out] File width in pixels
     * @param height [out] File height in pixels
     * @return true if valid header format, false on error
     */
    static bool ParseHeader(const QString& filepath, int& width, int& height);

    /**
     * @brief Parse header with detailed error reporting
     * @param filepath Path to metadata file
     * @param width [out] File width in pixels
     * @param height [out] File height in pixels
     * @param error [out] Detailed error information if parsing fails
     * @return true if valid header format, false on error
     */
    static bool ParseHeaderWithError(const QString& filepath, int& width, int& height,
                                   ImportError& error);

private:
    /**
     * @brief Process data stream with memory optimization
     * @param filepath Path to metadata file
     * @param settings Import configuration
     * @param width File width
     * @param height File height
     * @return Processed grayscale image
     */
    static QImage ProcessDataStream(const QString& filepath,
                                  const ImportSettings& settings,
                                  int width, int height);

    /**
     * @brief Process data stream with detailed error reporting
     * @param filepath Path to metadata file
     * @param settings Import configuration
     * @param width File width
     * @param height File height
     * @param error [out] Detailed error information if processing fails
     * @return Processed grayscale image
     */
    static QImage ProcessDataStreamWithError(const QString& filepath,
                                           const ImportSettings& settings,
                                           int width, int height,
                                           ImportError& error);

    /**
     * @brief Convert data value to grayscale uint8_t (0-255)
     * @param value Input data value
     * @param settings Import settings for range normalization
     * @return Grayscale value (0-255)
     */
    static uint8_t ConvertToGrayscale(double value, const ImportSettings& settings);

    /**
     * @brief Check if coordinates are within crop region
     * @param x X coordinate
     * @param y Y coordinate
     * @param settings Import settings with crop configuration
     * @return true if inside crop region or cropping disabled
     */
    static bool IsInCropRegion(int x, int y, const ImportSettings& settings);

    /**
     * @brief Validate header format and extract dimensions
     * @param header_parts Split header line components
     * @param width [out] Parsed width value
     * @param height [out] Parsed height value
     * @return true if valid format
     */
    static bool ValidateHeader(const QStringList& header_parts, int& width, int& height);

    /**
     * @brief Process range value according to out-of-range handling
     * @param input Raw input value
     * @param settings Import settings
     * @return Processed value within range
     */
    static double ProcessRangeValue(double input, const ImportSettings& settings);

    /**
     * @brief Create QImage from processed uint8_t data array
     * @param data Grayscale pixel data (0-255)
     * @param width Image width
     * @param height Image height
     * @return Grayscale QImage
     */
    static QImage CreateImageFromData(const uint8_t* data, int width, int height);

    // Constants
    static constexpr int MIN_DIMENSION = 2;
    static constexpr uint8_t GRAYSCALE_MIN = 0;
    static constexpr uint8_t GRAYSCALE_MAX = 255;
};

#endif // METADATAIMPORTER_H
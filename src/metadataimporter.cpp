#include "metadataimporter.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <cmath>
#include <algorithm>

QImage MetadataImporter::ImportMetadataFile(const QString& filepath,
                                          const ImportSettings& settings)
{
    // Parse header to get dimensions
    int width, height;
    if (!ParseHeader(filepath, width, height))
    {
        qWarning() << "Failed to parse header for file:" << filepath;
        return QImage();
    }

    // Process data stream with memory optimization
    return ProcessDataStream(filepath, settings, width, height);
}

bool MetadataImporter::ParseHeader(const QString& filepath, int& width, int& height)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Cannot open file:" << filepath;
        return false;
    }

    QTextStream stream(&file);
    if (stream.atEnd())
    {
        qWarning() << "Empty file:" << filepath;
        return false;
    }

    QString firstLine = stream.readLine().trimmed();
    QStringList parts = firstLine.split(' ', Qt::SkipEmptyParts);

    return ValidateHeader(parts, width, height);
}

bool MetadataImporter::ValidateHeader(const QStringList& header_parts, int& width, int& height)
{
    if (header_parts.size() != 2)
    {
        return false;
    }

    bool ok1, ok2;
    width = header_parts[0].toInt(&ok1);
    height = header_parts[1].toInt(&ok2);

    return ok1 && ok2 && width >= MIN_DIMENSION && height >= MIN_DIMENSION;
}

QImage MetadataImporter::ProcessDataStream(const QString& filepath,
                                         const ImportSettings& settings,
                                         int width, int height)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Cannot open file for data processing:" << filepath;
        return QImage();
    }

    QTextStream stream(&file);

    // Skip header line
    if (!stream.atEnd())
    {
        stream.readLine();
    }

    // Calculate effective dimensions after cropping
    int effective_width = width;
    int effective_height = height;
    int start_x = 0;
    int start_y = 0;

    if (settings.enable_cropping)
    {
        // Validate crop boundaries
        if (settings.crop_start_x < 0 || settings.crop_start_y < 0 ||
            settings.crop_end_x > width || settings.crop_end_y > height ||
            settings.crop_start_x >= settings.crop_end_x ||
            settings.crop_start_y >= settings.crop_end_y)
        {
            qWarning() << "Invalid crop boundaries";
            return QImage();
        }

        effective_width = settings.crop_end_x - settings.crop_start_x;
        effective_height = settings.crop_end_y - settings.crop_start_y;
        start_x = settings.crop_start_x;
        start_y = settings.crop_start_y;
    }

    // Allocate memory for processed data
    std::unique_ptr<uint8_t[]> pixel_data(new uint8_t[effective_width * effective_height]);

    int current_row = 0;
    int output_row = 0;

    // Process data line by line with streaming approach
    while (!stream.atEnd() && current_row < height)
    {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
        {
            qWarning() << "Empty line found at row:" << current_row;
            return QImage();
        }

        QStringList values = line.split(' ', Qt::SkipEmptyParts);
        if (values.size() != width)
        {
            qWarning() << "Row" << current_row << "has" << values.size()
                      << "values, expected" << width;
            return QImage();
        }

        // Check if this row is within crop region
        bool row_in_crop = !settings.enable_cropping ||
                          (current_row >= start_y && current_row < settings.crop_end_y);

        if (row_in_crop)
        {
            int output_col = 0;
            for (int col = 0; col < width; ++col)
            {
                // Check if this column is within crop region
                bool col_in_crop = !settings.enable_cropping ||
                                  (col >= start_x && col < settings.crop_end_x);

                if (col_in_crop)
                {
                    bool ok;
                    double value = values[col].toDouble(&ok);
                    if (!ok)
                    {
                        qWarning() << "Invalid numeric value at row" << current_row
                                  << "col" << col << ":" << values[col];
                        return QImage();
                    }

                    // Process range and convert to grayscale
                    double processed_value = ProcessRangeValue(value, settings);
                    uint8_t grayscale = ConvertToGrayscale(processed_value, settings);

                    // Store in output array
                    pixel_data[output_row * effective_width + output_col] = grayscale;
                    output_col++;
                }
            }
            output_row++;
        }

        current_row++;

        // Progress indicator for large files
        if (current_row % 1000 == 0)
        {
            QApplication::processEvents();
        }
    }

    if (current_row != height)
    {
        qWarning() << "Expected" << height << "data rows, found" << current_row;
        return QImage();
    }

    // Create QImage from processed data
    return CreateImageFromData(pixel_data.get(), effective_width, effective_height);
}

double MetadataImporter::ProcessRangeValue(double input, const ImportSettings& settings)
{
    if (input < settings.range_min)
    {
        switch (settings.out_of_range_handling)
        {
            case ImportSettings::CLAMP_TO_BOUNDS:
                return settings.range_min;
            case ImportSettings::SET_TO_ZERO:
                return settings.range_min;
            case ImportSettings::SET_TO_MAX:
                return settings.range_max;
        }
    }
    else if (input > settings.range_max)
    {
        switch (settings.out_of_range_handling)
        {
            case ImportSettings::CLAMP_TO_BOUNDS:
                return settings.range_max;
            case ImportSettings::SET_TO_ZERO:
                return settings.range_min;
            case ImportSettings::SET_TO_MAX:
                return settings.range_max;
        }
    }

    return input;
}

uint8_t MetadataImporter::ConvertToGrayscale(double value, const ImportSettings& settings)
{
    double range = settings.range_max - settings.range_min;
    if (range <= 0.0)
    {
        return GRAYSCALE_MIN;
    }

    double normalized = (value - settings.range_min) / range;
    normalized = std::max(0.0, std::min(1.0, normalized));

    return static_cast<uint8_t>(normalized * GRAYSCALE_MAX);
}

bool MetadataImporter::IsInCropRegion(int x, int y, const ImportSettings& settings)
{
    if (!settings.enable_cropping)
    {
        return true;
    }

    return x >= settings.crop_start_x && x < settings.crop_end_x &&
           y >= settings.crop_start_y && y < settings.crop_end_y;
}

QImage MetadataImporter::CreateImageFromData(const uint8_t* data, int width, int height)
{
    QImage image(width, height, QImage::Format_Grayscale8);

    for (int y = 0; y < height; ++y)
    {
        uint8_t* scan_line = image.scanLine(y);
        const uint8_t* source_line = &data[y * width];

        // Copy entire row at once for efficiency
        std::memcpy(scan_line, source_line, width);
    }

    return image;
}

QImage MetadataImporter::ImportMetadataFileWithError(const QString& filepath,
                                                   const ImportSettings& settings,
                                                   ImportError& error)
{
    // Initialize error
    error.type = ImportError::NO_ERROR;
    error.message.clear();
    error.row_number = -1;
    error.expected_count = -1;
    error.actual_count = -1;
    error.invalid_value.clear();

    // Parse header to get dimensions
    int width, height;
    if (!ParseHeaderWithError(filepath, width, height, error))
    {
        return QImage();
    }

    // Validate crop settings if enabled
    if (settings.enable_cropping)
    {
        if (settings.crop_start_x < 0 || settings.crop_start_y < 0 ||
            settings.crop_end_x > width || settings.crop_end_y > height ||
            settings.crop_start_x >= settings.crop_end_x ||
            settings.crop_start_y >= settings.crop_end_y)
        {
            error.type = ImportError::CROP_BOUNDARY_ERROR;
            error.message = QString("Crop region extends outside data boundaries.\n\n"
                                  "Data size: %1 x %2\n"
                                  "Crop region: (%3,%4) to (%5,%6)")
                                  .arg(width).arg(height)
                                  .arg(settings.crop_start_x).arg(settings.crop_start_y)
                                  .arg(settings.crop_end_x).arg(settings.crop_end_y);
            return QImage();
        }
    }

    // Process data stream
    return ProcessDataStreamWithError(filepath, settings, width, height, error);
}

bool MetadataImporter::ParseHeaderWithError(const QString& filepath, int& width, int& height,
                                           ImportError& error)
{
    // Initialize error
    error.type = ImportError::NO_ERROR;
    error.message.clear();

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        error.type = ImportError::FILE_NOT_FOUND;
        error.message = QString("Cannot open file: %1").arg(filepath);
        return false;
    }

    QTextStream stream(&file);
    if (stream.atEnd())
    {
        error.type = ImportError::INVALID_HEADER_FORMAT;
        error.message = "File is empty.\n\nExpected header format: 'width height'";
        return false;
    }

    QString firstLine = stream.readLine().trimmed();
    QStringList parts = firstLine.split(' ', Qt::SkipEmptyParts);

    if (parts.size() != 2)
    {
        error.type = ImportError::INVALID_HEADER_FORMAT;
        error.message = QString("Header must contain two integers: 'width height'\n\n"
                              "Found: '%1'").arg(firstLine);
        return false;
    }

    bool ok1, ok2;
    width = parts[0].toInt(&ok1);
    height = parts[1].toInt(&ok2);

    if (!ok1 || !ok2)
    {
        error.type = ImportError::INVALID_HEADER_FORMAT;
        error.message = QString("Header values must be integers.\n\n"
                              "Found: '%1'").arg(firstLine);
        return false;
    }

    if (width < MIN_DIMENSION || height < MIN_DIMENSION)
    {
        error.type = ImportError::INVALID_DIMENSIONS;
        error.message = QString("Width and height must be >= %1\n\n"
                              "Found: width=%2, height=%3")
                              .arg(MIN_DIMENSION).arg(width).arg(height);
        return false;
    }

    return true;
}

QImage MetadataImporter::ProcessDataStreamWithError(const QString& filepath,
                                                  const ImportSettings& settings,
                                                  int width, int height,
                                                  ImportError& error)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        error.type = ImportError::FILE_NOT_FOUND;
        error.message = QString("Cannot open file for data processing: %1").arg(filepath);
        return QImage();
    }

    QTextStream stream(&file);

    // Skip header line
    if (!stream.atEnd())
    {
        stream.readLine();
    }

    // Calculate effective dimensions after cropping
    int effective_width = width;
    int effective_height = height;
    int start_x = 0;
    int start_y = 0;

    if (settings.enable_cropping)
    {
        effective_width = settings.crop_end_x - settings.crop_start_x;
        effective_height = settings.crop_end_y - settings.crop_start_y;
        start_x = settings.crop_start_x;
        start_y = settings.crop_start_y;
    }

    // Allocate memory for processed data
    std::unique_ptr<uint8_t[]> pixel_data(new uint8_t[effective_width * effective_height]);

    int current_row = 0;
    int output_row = 0;

    // Process data line by line with streaming approach
    while (!stream.atEnd() && current_row < height)
    {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
        {
            error.type = ImportError::DATA_MISMATCH;
            error.message = QString("Empty line found at row %1.\n\n"
                                  "All data rows must contain numeric values.")
                                  .arg(current_row + 1);
            error.row_number = current_row + 1;
            return QImage();
        }

        QStringList values = line.split(' ', Qt::SkipEmptyParts);
        if (values.size() != width)
        {
            error.type = ImportError::DATA_MISMATCH;
            error.message = QString("Expected %1 data rows, found %2 rows\n"
                                  "Expected %3 columns per row, found %4 columns in row %5")
                                  .arg(height).arg(current_row)
                                  .arg(width).arg(values.size()).arg(current_row + 1);
            error.row_number = current_row + 1;
            error.expected_count = width;
            error.actual_count = values.size();
            return QImage();
        }

        // Check if this row is within crop region
        bool row_in_crop = !settings.enable_cropping ||
                          (current_row >= start_y && current_row < settings.crop_end_y);

        if (row_in_crop)
        {
            int output_col = 0;
            for (int col = 0; col < width; ++col)
            {
                // Check if this column is within crop region
                bool col_in_crop = !settings.enable_cropping ||
                                  (col >= start_x && col < settings.crop_end_x);

                if (col_in_crop)
                {
                    bool ok;
                    double value = values[col].toDouble(&ok);
                    if (!ok)
                    {
                        error.type = ImportError::INVALID_NUMERIC_DATA;
                        error.message = QString("Non-numeric data found in row %1: '%2'\n\n"
                                              "All data must be integers or floating point numbers")
                                              .arg(current_row + 1).arg(values[col]);
                        error.row_number = current_row + 1;
                        error.invalid_value = values[col];
                        return QImage();
                    }

                    // Process range and convert to grayscale
                    double processed_value = ProcessRangeValue(value, settings);
                    uint8_t grayscale = ConvertToGrayscale(processed_value, settings);

                    // Store in output array
                    pixel_data[output_row * effective_width + output_col] = grayscale;
                    output_col++;
                }
            }
            output_row++;
        }

        current_row++;

        // Progress indicator for large files
        if (current_row % 1000 == 0)
        {
            QApplication::processEvents();
        }
    }

    if (current_row != height)
    {
        error.type = ImportError::DATA_MISMATCH;
        error.message = QString("Expected %1 data rows, found %2 rows")
                              .arg(height).arg(current_row);
        error.expected_count = height;
        error.actual_count = current_row;
        return QImage();
    }

    // Create QImage from processed data
    return CreateImageFromData(pixel_data.get(), effective_width, effective_height);
}
#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QString>
#include <QImage>
#include <QFile>
#include <QTextStream>
#include <QTemporaryDir>

#include "metadataimporter.h"

class MetadataImporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        argc_ = 1;
        argv_ = new char*[1];
        argv_[0] = const_cast<char*>("test");
        app_ = new QCoreApplication(argc_, argv_);

        temp_dir_ = new QTemporaryDir();
        ASSERT_TRUE(temp_dir_->isValid());

        CreateTestDataFiles();
    }

    void TearDown() override {
        delete temp_dir_;
        delete app_;
        delete[] argv_;
    }

    void CreateTestDataFiles() {
        // test_data_4x3.txt - valid 4x3 data file
        {
            QFile file(temp_dir_->filePath("test_data_4x3.txt"));
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&file);
            out << "4 3\n";
            out << "10.5 20.0 -5.0 100.0\n";
            out << "15.0 25.0 0.0 95.0\n";
            out << "12.0 30.0 -10.0 80.0\n";
        }

        // test_invalid_header.txt - invalid header format
        {
            QFile file(temp_dir_->filePath("test_invalid_header.txt"));
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&file);
            out << "abc def\n";
            out << "10.5 20.0 -5.0 100.0\n";
            out << "15.0 25.0 0.0 95.0\n";
        }

        // test_non_numeric.txt - contains non-numeric data
        {
            QFile file(temp_dir_->filePath("test_non_numeric.txt"));
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&file);
            out << "4 3\n";
            out << "10.5 20.0 -5.0 100.0\n";
            out << "15.0 abc 0.0 95.0\n";
            out << "12.0 30.0 -10.0 80.0\n";
        }

        // test_wrong_dimensions.txt - row has wrong number of columns
        {
            QFile file(temp_dir_->filePath("test_wrong_dimensions.txt"));
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&file);
            out << "4 3\n";
            out << "10.5 20.0 -5.0 100.0 50.0\n";
            out << "15.0 25.0 0.0 95.0\n";
            out << "12.0 30.0 -10.0 80.0\n";
        }
    }

    QString GetTestFilePath(const QString& filename) {
        return temp_dir_->filePath(filename);
    }

    int argc_;
    char** argv_;
    QCoreApplication* app_;
    QTemporaryDir* temp_dir_;
};

TEST_F(MetadataImporterTest, ValidHeaderParsing) {
    int width, height;
    MetadataImporter::ImportError error;

    bool result = MetadataImporter::ParseHeaderWithError(
        GetTestFilePath("test_data_4x3.txt"), width, height, error);

    EXPECT_TRUE(result);
    EXPECT_EQ(error.type, MetadataImporter::ImportError::NO_ERROR);
    EXPECT_EQ(width, 4);
    EXPECT_EQ(height, 3);
}

TEST_F(MetadataImporterTest, InvalidHeaderFormat) {
    int width, height;
    MetadataImporter::ImportError error;

    bool result = MetadataImporter::ParseHeaderWithError(
        GetTestFilePath("test_invalid_header.txt"), width, height, error);

    EXPECT_FALSE(result);
    EXPECT_EQ(error.type, MetadataImporter::ImportError::INVALID_HEADER_FORMAT);
    EXPECT_FALSE(error.message.isEmpty());
}

TEST_F(MetadataImporterTest, FileNotFound) {
    int width, height;
    MetadataImporter::ImportError error;

    bool result = MetadataImporter::ParseHeaderWithError(
        "nonexistent_file.txt", width, height, error);

    EXPECT_FALSE(result);
    EXPECT_EQ(error.type, MetadataImporter::ImportError::FILE_NOT_FOUND);
    EXPECT_FALSE(error.message.isEmpty());
}

TEST_F(MetadataImporterTest, ValidDataImport) {
    MetadataImporter::ImportSettings settings;
    settings.range_min = 0.0;
    settings.range_max = 100.0;
    settings.out_of_range_handling = MetadataImporter::ImportSettings::CLAMP_TO_BOUNDS;
    settings.enable_cropping = false;

    MetadataImporter::ImportError error;

    QImage image = MetadataImporter::ImportMetadataFileWithError(
        GetTestFilePath("test_data_4x3.txt"), settings, error);

    EXPECT_FALSE(image.isNull());
    EXPECT_EQ(error.type, MetadataImporter::ImportError::NO_ERROR);
    EXPECT_EQ(image.width(), 4);
    EXPECT_EQ(image.height(), 3);
    EXPECT_EQ(image.format(), QImage::Format_Grayscale8);
}

TEST_F(MetadataImporterTest, WrongDimensions) {
    MetadataImporter::ImportSettings settings;
    settings.range_min = 0.0;
    settings.range_max = 100.0;
    settings.out_of_range_handling = MetadataImporter::ImportSettings::CLAMP_TO_BOUNDS;
    settings.enable_cropping = false;

    MetadataImporter::ImportError error;

    QImage image = MetadataImporter::ImportMetadataFileWithError(
        GetTestFilePath("test_wrong_dimensions.txt"), settings, error);

    EXPECT_TRUE(image.isNull());
    EXPECT_EQ(error.type, MetadataImporter::ImportError::DATA_MISMATCH);
    EXPECT_GT(error.row_number, 0);
}

TEST_F(MetadataImporterTest, NonNumericData) {
    MetadataImporter::ImportSettings settings;
    settings.range_min = 0.0;
    settings.range_max = 100.0;
    settings.out_of_range_handling = MetadataImporter::ImportSettings::CLAMP_TO_BOUNDS;
    settings.enable_cropping = false;

    MetadataImporter::ImportError error;

    QImage image = MetadataImporter::ImportMetadataFileWithError(
        GetTestFilePath("test_non_numeric.txt"), settings, error);

    EXPECT_TRUE(image.isNull());
    EXPECT_EQ(error.type, MetadataImporter::ImportError::INVALID_NUMERIC_DATA);
    EXPECT_GT(error.row_number, 0);
    EXPECT_FALSE(error.invalid_value.isEmpty());
}

TEST_F(MetadataImporterTest, CroppingFunctionality) {
    MetadataImporter::ImportSettings settings;
    settings.range_min = 0.0;
    settings.range_max = 100.0;
    settings.out_of_range_handling = MetadataImporter::ImportSettings::CLAMP_TO_BOUNDS;
    settings.enable_cropping = true;
    settings.crop_start_x = 1;
    settings.crop_start_y = 1;
    settings.crop_end_x = 3;
    settings.crop_end_y = 2;

    MetadataImporter::ImportError error;

    QImage image = MetadataImporter::ImportMetadataFileWithError(
        GetTestFilePath("test_data_4x3.txt"), settings, error);

    EXPECT_FALSE(image.isNull());
    EXPECT_EQ(error.type, MetadataImporter::ImportError::NO_ERROR);
    EXPECT_EQ(image.width(), 2);  // crop_end_x - crop_start_x = 3 - 1 = 2
    EXPECT_EQ(image.height(), 1); // crop_end_y - crop_start_y = 2 - 1 = 1
}

TEST_F(MetadataImporterTest, CropBoundaryError) {
    MetadataImporter::ImportSettings settings;
    settings.range_min = 0.0;
    settings.range_max = 100.0;
    settings.out_of_range_handling = MetadataImporter::ImportSettings::CLAMP_TO_BOUNDS;
    settings.enable_cropping = true;
    settings.crop_start_x = 0;
    settings.crop_start_y = 0;
    settings.crop_end_x = 10;  // Beyond data boundary (width=4)
    settings.crop_end_y = 10;  // Beyond data boundary (height=3)

    MetadataImporter::ImportError error;

    QImage image = MetadataImporter::ImportMetadataFileWithError(
        GetTestFilePath("test_data_4x3.txt"), settings, error);

    EXPECT_TRUE(image.isNull());
    EXPECT_EQ(error.type, MetadataImporter::ImportError::CROP_BOUNDARY_ERROR);
    EXPECT_FALSE(error.message.isEmpty());
}

TEST_F(MetadataImporterTest, RangeProcessing) {
    MetadataImporter::ImportSettings settings;
    settings.range_min = 20.0;
    settings.range_max = 80.0;
    settings.out_of_range_handling = MetadataImporter::ImportSettings::CLAMP_TO_BOUNDS;
    settings.enable_cropping = false;

    MetadataImporter::ImportError error;

    QImage image = MetadataImporter::ImportMetadataFileWithError(
        GetTestFilePath("test_data_4x3.txt"), settings, error);

    EXPECT_FALSE(image.isNull());
    EXPECT_EQ(error.type, MetadataImporter::ImportError::NO_ERROR);
    EXPECT_EQ(image.width(), 4);
    EXPECT_EQ(image.height(), 3);

    // Test with zero handling
    settings.out_of_range_handling = MetadataImporter::ImportSettings::SET_TO_ZERO;

    QImage image2 = MetadataImporter::ImportMetadataFileWithError(
        GetTestFilePath("test_data_4x3.txt"), settings, error);

    EXPECT_FALSE(image2.isNull());
    EXPECT_EQ(error.type, MetadataImporter::ImportError::NO_ERROR);
}
